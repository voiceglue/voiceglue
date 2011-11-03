#                        -*-CPerl-*-
package Cam::Scom;

require DynaLoader;
@ISA = qw(DynaLoader);

use Exporter 'import';
@EXPORT_OK = qw(scomrunin scomrun scomrunin_opt scomrun_opt);

use POSIX qw(EAGAIN EINTR EBADF EINPROGRESS EALREADY dup2 dup);
use Socket qw(AF_INET SOCK_DGRAM SOCK_STREAM INADDR_LOOPBACK
              SOL_SOCKET SO_REUSEADDR SO_BROADCAST
              unpack_sockaddr_in pack_sockaddr_in inet_aton);
use Fcntl qw(O_NDELAY F_GETFL F_SETFL F_GETFD F_SETFD);
use BSD::Resource;

use Cam::Scom::Fh;
use Cam::Scom::Event;

use warnings;
use bytes;
use integer;
use strict;

use constant Private_fh_prefix => "XXCOMTMPXX";
use constant MY_FH_PREFIX => "XXCOMTMPXX";
use constant Blocksize => 8192;
use constant MAXBUFSIZE => 1024 * 1024;

=head1 NAME

Cam::Scom - Supercharged asynchronous inter-process COMmunication

=head1 VERSION

Version 0.08

=cut

our $VERSION = '0.08';

=head1 SYNOPSIS

Cam::Scom provides two levels of functionality.

Its high level functions provide advanced means of interacting with
executables.  They allow execution with feeds to stdin and capture of
stdout and stderr.  The high level functions are:

    scomrun (@command)
    scomrunin ($input, @command)
    scomrun_opt ($uid, $gid, $detach, @command)
    scomrunin_opt ($uid, $gid, $detach, $input, @command)

Its low level functions provide asynchronous manipulation of reads and
writes on multiple filehandles within a single thread and process.  All
other functions are low level functions.  They are used in an object-
oriented fashion.

Here is  a  code fragment that  performs the equivalent  of the  shell
command "man perl | col -b":

	$scom->exec (["MANIN<1?"], 0, "man", "perl");
	$scom->exec (["MANIN>0!"], 0, "col", "-b");

=cut

=head2 LOW LEVEL FUNCTION OVERVIEW

At a low level, Scom provides event-driven inter-process communication
over file descriptors.  The supported types of descriptors are pipe
streams, socket streams, socket servers, and datagrams.  Scom supports
streams both to and from other executing processes without danger of
deadlock.  Deadlock is avoided by using non-blocking I/O and buffering
all blocked reads and writes for later retry.  It is necessary to have
an event processing loop that regularly calls getevents() to maintain
data flow.  Typically getevents() is called in the main dispatching
loop to provide a means of blocking until the next I/O event occurs.

The Scom package will only manipulate file descriptors that have been
registered with it.  They may be created outside of Scom and then
registered, or they may be created by methods provided by Scom.
Provided methods include exec() for creating and registering pipes to
and from exec'd processes, connect() for creating and registering TCP
client sockets, listen() for creating and registering TCP server
sockets, and datagram() for creating and registering UDP sockets.
Method register() will register any previously opened file descriptors.
Previously opened filehandles must support fileno, close, and
the following additional methods depending on their type:

  streams:           read, write  (filehandles or file descriptors)
  datagrams:         recv, send   (requires perl filehandles)
  socket listeners:  accept       (requires perl filehandles)

Finally, expect() implements all of the functionality of the
tcl-based expect language, with the exception of using actual pty's.

Filehandles registered with Scom may be either old-style perl
filehandles or FileHandle or IO::Handle objects or Unix file
descriptor numbers.  If old-style perl filehandles are used, they are
always utilized as symbolic references, and are evaluated relative to
the Scom package.  This implies that it is usually best to include a
full package name with the name of a filehandle, such as:

	$obj->register ("main::READER<");

Registered filehandles should not be manipulated outside of the Scom
routines.

Any number of Scom objects may be created, each with a different set
of registered filehandles.  Do not register a filehandle to
more than one Scom object.

=cut

##  Concepts and Definitions:
##    fh object        -- The Cam::Scom::Fh object representing the filehandle.
##    fhname           -- A string that uniquely maps to the Cam::Scom::Fh.
use fields
(
 "fhs",               ## List of all Fh objects for all registered filehandles
 "fhname_to_index",   ## Maps registered fhnames to index in the "fhs" array
 "fh_has_event",      ## Maps registered fhnames if
                      ##    a not-yet-returned event has occurred on
                      ##    fh, which is
                      ##    either EOF/close, error, or readable data.
 "fd_to_fh",          ## Maps file descriptors for fh's that have them to
                      ##     their Cam::Scom::Fh, o.w. undef
 "log_dir",           ## Directory to log all traffic
 "read_mask",         ## The current read select mask
 "write_mask",        ## The current write select mask
 "error_mask",        ## The current error select mask
 "all_mask",          ## The fd mask of all registered filehandles
 "fileno_limit",      ## The maximum number of file descriptors
 "empty_mask",        ## An empty select mask
);

=head1 FUNCTIONS

=head2 scomrun (@command)

All of the functions scomrun, scomrunin, scomrun_opt,
and scomrunin_opt execute @command via perl's exec(), and return the
following array:

	($ok, $msg, $stdout_output, $stderr_output)

where $stdout_output is all the output from the command's
stdout, and $stderr_output is all the output from the
command's stderr.  Also sets $?  to the exit status.  On error
it sets $ok to false, $msg to an error message, and other
results to undef.  The command is run in the parent's process
group.

The $input parameter to scomrunin() and scomrunin_opt() is fed
to @command's stdin.  scomrun() and scomrun_opt() inherit the
existing stdin.

For the *_opt versions, if either $uid or $gid are defined,
sets the corresponding uid = $uid and/or gid = $gid (both real
and effective) in the child process.  If $detach is true, the
command will have its own process group.

The SIGCHLD signal, if not set to the default behavior, will
be set to its default behavior before execution, and then
reset to its original value when done.  Note that this can
result in signal lossage unless handled correctly.

These functions will not return until the output pipes stderr
and stdout have closed, whether or not the process has exited.
This means that if the started process starts children that
inherit stdout and/or stderr, the functions will not return
until all such children (recursively) have either terminated
or closed their stdout & stderr descriptors.

=cut

sub scomrun
{
    my (@command) = @_;
    return (_do_execute (undef, undef, 0, 0, "", @command));
};

=head2 scomrunin ($input, @command)

See scomrun.

=cut

sub scomrunin
{
    my ($input) = shift (@_);
    my (@command) = @_;
    return (_do_execute (undef, undef, 0, 1, $input, @command));
};

=head2 scomrun_opt ($uid, $gid, $detach, @command)

See scomrun.

=cut

sub scomrun_opt
{
    my ($uid, $gid, $detach, @command) = @_;
    return (_do_execute ($uid, $gid, $detach, 0, "", @command));
};

=head2 scomrunin_opt ($uid, $gid, $detach, $input, @command)

See scomrun.

=cut

sub scomrunin_opt
{
    my ($uid) = shift (@_);
    my ($gid) = shift (@_);
    my ($detach) = shift (@_);
    my ($input) = shift (@_);
    my (@command) = @_;
    return (_do_execute ($uid, $gid, $detach, 1, $input, @command));
};

=head2 new

Returns a new, empty Scom object with no registered
filehandles on success.
On failure, returns a string describing the error.
Use ref() to determine success on the result.

=cut

sub new
{
    my ($rlimit_cur, $rlimit_max);
    my ($resource) = RLIMIT_NOFILE;
    my $self = shift;

    (ref $self) || ($self = fields::new($self));
    $self->{fhs} = [];
    $self->{fhname_to_index} = {};
    $self->{fh_has_event} = {};
    $self->{fd_to_fh} = [];
    $self->{log_dir} = undef;

    ##  Compute fileno_limit
    scalar (($rlimit_cur, $rlimit_max) = getrlimit ($resource))
      || return ("Cannot create Cam::Scom object: getrlimit failed $!");
    $self->{fileno_limit} = $rlimit_cur;

    $self->{empty_mask} = "\x00" x int ($rlimit_cur / 8);
    $self->{read_mask} = $self->{empty_mask};
    $self->{write_mask} = $self->{empty_mask};
    $self->{error_mask} = $self->{empty_mask};
    $self->{all_mask} = $self->{empty_mask};
    return ($self);
};

=head1 METHODS

=head2 ->register ($pipe [, $pipe ...])

	Registers filehandles that are already opened.  The format for
	each $pipe element is either:

		"<fhname><direction>[<exec>][<flags>]"
        or:
		[<FileHandle>, "<direction>[<exec>][<flags>]"]

	where
	    <fhname>	::=	the filehandle symbolic name or descriptor

	    <FileHandle>::=     the FileHandle object or name or descriptor

	    <direction>	::= 	">" for write-only streams,
				"<" for read-only streams,
				"+" for read-write stream (bidirectional),
				"*" for sendto-recvfrom (bidirectional).

	    <exec>	::=	(Ignored by register, but used
				by exec.  See exec.)

	    <flags>	::=	flags indicating the setting of
				close-on-unregister and the value
				of the input record separator.
				Has form:

				<no_close><close-on-exec><separator>

		<no_close>	::=	either "", indicating close-on-
					unregister, or "|", indicating
					no close-on-unregister.

		<close-on-exec>	::=	either "", meaning don't set
					close-on-exec, or ".", meaning
					set close-on-exec.

		<separator>	::=	the input record separator
					for getevents (see below).
					If omitted, the separator
					will be "\n".  Otherwise,
					specify as ":<chars>" where
					<chars> is the (possibly empty)
					separator character sequence.
					Input record separators are only
					meaningful on stream types, and
					are ingnored on datagram sockets.

	It is an error to register a filehandle first in one direction
	and subsequently in the other; both directions must be done at
	the same time.

	Returns ($ok,  $msg) where  $ok is  true  on success, false on
	error.  On error, $msg will be set to an error message.

=cut

sub register
{
    my Cam::Scom $this = shift (@_);
    my (@pipes) = @_;
    my (@new_fhs) = ();
    my (%new_names) = ();
    my ($pipe, $i, $ok, $msg, $fhmap, $type, $fhandle, $fhname, $direction);
    my ($fhname_to_index, $fd_to_fh, $fhs, $index, $fh_inuse);
    my Cam::Scom::Fh $fh;

    $fhname_to_index = $this->{fhname_to_index};
    $fd_to_fh = $this->{fd_to_fh};
    $fhs = $this->{fhs};

    ##  Parse out all the requests
    foreach $pipe (@pipes)
    {
	($ok, $msg, $fh) = _parse_pipe ($pipe);

	##  See if already registered
	$fhname = $fh->{fh};
	if (defined ($new_names{$fhname}))
	{
	    return (0, "Filehandle \"$fhname\" passed multiple times to comregister");
	};
	if (defined ($fhname_to_index->{$fhname}))
	{
	    return (0, "Filehandle \"$fhname\" already registered");
	};

	##  Extract the fd
	$fhandle = $fh->{handle};
	if ($fh->{is_object})
	{
	    $fh->{fd} = $fhandle->fileno();
	}
	elsif ($fh->{is_fd_only})
	{
	    $fh->{fd} = $fhandle + 0;
	}
	else
	{
	    no strict "refs";
	    $fh->{fd} = fileno ($fhandle);
	    use strict "refs";
	};
	if (! defined ($fh->{fd}))
	{
	    return (0, "Filehandle \"$fhname\" returns undef from fileno()");
	};
	if ($fh->{fd} >= $this->{fileno_limit})
	{
	    return (0, "Filehandle \"$fhname\" has fd = " . $fh->{fd} .
		    " which is >= fileno limit of " . $this->{fileno_limit});
	};
	if (defined ($fh_inuse = $fd_to_fh->[$fh->{fd}]))
	{
	    return (0, "Filehandle \"$fhname\" has fd = " . $fh->{fd} .
		    " which is already in use by \"" .
		    $fh_inuse->{fh} . "\"");
	};

	##  Set non-blocking
	if (! (($ok, $msg) = _set_blocking ($fh->{fd}, 0))[0])
	{
	    return ($ok, $msg);
	};

	##  Set close-on-exec
	if ($fh->{close_on_exec})
	{
	    if (! (($ok, $msg) = _set_close_on_exec ($fh->{fd}, 1))[0])
	    {
		return ($ok, $msg);
	    };
	};

	##  Set the rest of the fields
	$fh->{read_open} = 0;
	$fh->{write_open} = 0;
	$fh->{read_buf} = [];
	$fh->{read_buf_size} = 0;
	$fh->{read_buf_limit} = MAXBUFSIZE;
	$fh->{read_buf_killer} = 0;
	$fh->{write_buf} = [];
	$fh->{write_buf_size} = 0;
	$fh->{write_buf_limit} = MAXBUFSIZE;
	$fh->{write_buf_killer} = 0;
	$fh->{write_buf_killmsg} = undef;
	$fh->{read_server} = 0;
	$fh->{client_base} = undef;
	$fh->{close_on_flush} = 0;
	$fh->{read_logfile} = undef;
	$fh->{write_logfile} = undef;
	$direction = $fh->{direction};
	if (($direction eq "<") || ($direction eq "+") || ($direction eq "*"))
	{
	    $fh->{read_open} = 1;
	};
	if (($direction eq ">") || ($direction eq "+") || ($direction eq "*"))
	{
	    $fh->{write_open} = 1;
	};
	if ($fh->{messaging} = ($direction eq "*"))
	{
	    $fh->{read_separator} = "";
	    $fh->{separator_len} = 0;
	};

	##  Put in the list to be registered
	push (@new_fhs, $fh);
	$new_names{$fhname} = 1;
    };

    ##  Now record all the newly registered filehandles
    $i = scalar (@{$this->{fhs}});
    foreach $fh (@new_fhs)
    {
	if ($fh->{read_open})
	{
	    vec ($this->{read_mask}, $fh->{fd}, 1) = 1;
	    vec ($this->{error_mask}, $fh->{fd}, 1) = 1;
	};
	$fhs->[$i] = $fh;
	$fhname_to_index->{$fh->{fh}} = $i;
	$fd_to_fh->[$fh->{fd}] = $fh;
	vec ($this->{all_mask}, $fh->{fd}, 1) = 1;
	++$i;

	##  DEBUG
##	print STDOUT
##	  ("Registered fd=" . $fh->{fd} . " (" . $fh->{fh} . ")\n");
    };

    return (1, "");
};

=head2 ->unregister ($filehandle [, $filehandle ...])

Unregisters  the  specified   filehandles.    Unregistering  a
filehandle closes it, unless close-on-unregister was unset.

=cut

sub unregister
{
    my Cam::Scom $this = shift (@_);
    my (@fhs_to_unregister) = @_;
    my ($fhname, $fhandle, $index, $i, $len, $is_object, $fd,
	$fhname_to_index, $fd_to_fh, $fhs);
    my Cam::Scom::Fh $fh;

    $fhs = $this->{fhs};
    $fhname_to_index = $this->{fhname_to_index};
    $fd_to_fh = $this->{fd_to_fh};
    foreach $fhandle (@fhs_to_unregister)
    {
	defined ($fhandle) || next;
	$fhname = _fh_to_name ($fhandle);
	if (defined ($index = $fhname_to_index->{$fhname}))
	{
	    $fh = $fhs->[$index];
	    if ($fh->{close_on_unreg})
	    {
		if ($fh->{is_object})
		{
		    $fh->{handle}->close();
		}
		elsif ($fh->{is_fd_only})
		{
		    POSIX::close ($fhandle);
		}
		else
		{
		    no strict "refs";
		    close ($fh->{handle});
		    use strict "refs";
		};
	    };

	    ##  Knock it out of the top-level structures
	    if (defined ($fd = $fh->{fd}))
	    {
		##  DEBUG
##		print STDOUT ("Removing fd=" . $fd . " (" . $fh->{fh}
##			      . ") via unregister\n");

		vec ($this->{read_mask}, $fd, 1) = 0;
		vec ($this->{write_mask}, $fd, 1) = 0;
		vec ($this->{error_mask}, $fd, 1) = 0;
		vec ($this->{all_mask}, $fd, 1) = 0;
		$fd_to_fh->[$fd] = undef;
	    };
	    delete $fhname_to_index->{$fhname};
	    delete $this->{fh_has_event}{$fhname};
	    splice (@$fhs, $index, 1);
	    $len = scalar (@$fhs);
	    for ($i = $index; $i < $len; ++$i)
	    {
		$fh = $fhs->[$i];
		$fhname_to_index->{$fh->{fh}} = $i;
	    };
	};
    };

    return (1);
};

=head2 ->exec_opt (@$pipes, $uid, $gid, $detach, @command)

Runs @command via perl's exec(), and attaches input and output
pipes to it.  If $detach is true, the command will have its
own process group.  $pipes is a ref to an array of strings
defining the pipes to connect to @command.  The format for
each pipes element is as in register, except that <exec> is
required, and <direction> must be ">" or "<".  <exec> is
defined as:

	    <exec>	::=	<fd><use>

	    <fd>	::=	the file descriptor number within
				the exec'd command to attach to.

	    <use>	::=	either "", indicating that <fhname>
				is to be created and registered,
				or "?", indicating that <fhname> is to
				be created but not registered (though
				the close-on-exec flag is still valid),
				or "!", indicating that <fhname> already
				exists, and should be usurped by the
				exec'd command, and closed in this process.

Both return ($pid, $msg), where $pid is the pid from the perl
fork() call, or 0 on error.  On error, $msg will be set to an
error message.

For the exec_opt() version, if either $uid or $gid are
defined, sets the corresponding uid = $uid and/or gid = $gid
(both real and effective) in the child process.

The descriptors passed to the exec'd process will have flags
as originally set by the system pipe() call (and thus are
blocking).

=cut

sub exec_opt
{
    my Cam::Scom $this = shift (@_);
    my ($pipes) = shift (@_);
    my ($uid) = shift (@_);
    my ($gid) = shift (@_);
    my ($detach) = shift (@_);
    my (@command) = @_;

    my Cam::Scom::Fh $fh;
    my (@direction, @fhobj, @childfh, @childfd, @use, @close_on_exec);
    my (@pipes_to_register, @pipes_to_return);
    my ($direction, $fd, $use, $fhandle);
    my ($i, $r, $holder_index, $pid, $j, $old_fd, $new_fd);
    my ($p_spec, $type, $fd_spec, $handle);
    my ($tmpfhnum) = 0;
    my ($tmpfh, $tmpfhname);
    my ($ok, $msg, $fhmap);
    
    ##  Create all the pipes, setting:
    ##  @direction == ">" or "<" specified for each pipe (relative to parent)
    ##  @fhobj == parent filehandle object to create and return
    ##  @childfh == temporary child filehandle name given to each child pipe
    ##  @childfd == file descriptor number to ultimately assign each child pipe
    ##  @use == "" or "?" or "!" for create&register, create, or use.
    ##  @close_on_exec == the close_on_exec flag
    foreach $p_spec (@$pipes)
    {
	## Parse out the fields
	if (! (($ok, $msg, $fh) = _parse_pipe ($p_spec))[0])
	{
	    for ($i = 0; $i < scalar (@fhobj); ++$i)
	    {
		no strict "refs";
		defined ($fhobj[$i]) && close ($fhobj[$i]);
		close ($childfh[$i]);
		use strict "refs";
	    };
	    return (0, $msg);
	};
	$handle = $fh->{handle};
	$direction = $fh->{direction};

	##  Do the pipe
	push (@direction, $direction);
	push (@childfd, $fh->{child_fd});
	push (@use, $fh->{use});
	push (@close_on_exec, $fh->{close_on_exec});
	if ($fh->{use} ne "!")
	{
	    ##  Create a new pipe
	    push (@fhobj, $handle);
	    $tmpfh = MY_FH_PREFIX . $tmpfhnum;  ++$tmpfhnum;
	    push (@childfh, $tmpfh);
	    if ($direction eq ">")
	    {
		no strict "refs";
		$r = pipe ($tmpfh, $handle);
		use strict "refs";
	    }
	    else
	    {
		no strict "refs";
		$r = pipe ($handle, $tmpfh);
		use strict "refs";
	    };
	    if (! $r)
	    {
		for ($i = 0; $i < scalar (@fhobj); ++$i)
		{
		    no strict "refs";
		    defined ($fhobj[$i]) && close ($fhobj[$i]);
		    close ($childfh[$i]);
		    use strict "refs";
		};
		if ($direction eq ">")
		{
		    return (0, "pipe($tmpfh, $handle) returned $r: $!");
		}
		else
		{
		    return (0, "pipe($handle, $tmpfh) returned $r: $!");
		};
	    };
	}
	else
	{
	    ##  Use existing pipe
	    push (@fhobj, undef);
	    push (@childfh, $handle);
	};
    };
    
    ##  Do the fork
    if (! ($pid = fork()))
    {
	if (!defined ($pid))
	{
	    ##  fork() failed
	    for ($i = 0; $i < scalar (@fhobj); ++$i)
	    {
		no strict "refs";
		defined ($fhobj[$i]) && close ($fhobj[$i]);
		close ($childfh[$i]);
		use strict "refs";
	    };
	    return (0, "fork() failed: $!");
	};
	
	##  I am the child
	
	##  Close parent filehandles
	foreach $fhandle (@fhobj)
	{
	    no strict "refs";
	    defined ($fhandle) && close ($fhandle);
	    use strict "refs";
	};
	
	##  Perform dup2s to correct fd numbers.
	for ($i = 0; $i < scalar (@childfh); ++$i)
	{
	    no strict "refs";
	    $old_fd = fileno($childfh[$i]);
	    use strict "refs";
	    $new_fd = $childfd[$i];
	    
	    ##  Have to be a little careful here.
	    ##  There is no guarantee that the new_fd is not
	    ##  currently holding a temporary pipe end.
	    ##  If so, dup the temporary fd to another fd.
	    if (_is_fd_used ($new_fd))
	    {
		##  Find out if one of my temporaries is holding this fd
		undef ($holder_index);
		for ($j = $i + 1; $j < scalar (@childfh); ++$j)
		{
		    no strict "refs";
		    (fileno ($childfh[$j]) == $new_fd)
			&& ($holder_index = $j);
		    use strict "refs";
		};
		if (defined ($holder_index))
		{
		    ##  Dup the temporary to another fd
		    $fhandle = $childfh[$holder_index];
		    $direction = $direction[$holder_index];
		    ($direction eq "<")
			&& ($direction = ">")
			    || ($direction = "<");
		    $tmpfh = MY_FH_PREFIX . $tmpfhnum;
		    ++$tmpfhnum;
		    no strict "refs";
		    (open ($tmpfh, "$direction&$fhandle"))
			|| &_child_safe_die
			    ("open ($tmpfh, \"$direction&$fhandle\") failed: $!");
		    close ($fhandle);
		    use strict "refs";
		    $childfh[$holder_index] = $tmpfh;
		};
	    };
	    
	    ##  Perform the dup2
	    if ($old_fd != $new_fd)
	    {
		$r = dup2 ($old_fd, $new_fd);
		($r eq "0 but true") && ($r = 0);
		($r == $new_fd)
		    || &_child_safe_die
			("dup2($old_fd, $new_fd) returned $r: $!");
		no strict "refs";
		close ($childfh[$i]);
		use strict "refs";
	    };
	    
	    ##  Unset close-on-exec flag
	    $tmpfhname = $childfh[$i] . "DUP2";
	    if ($direction[$i] eq ">")
	    {
		##  This is a read handle
		no strict "refs";
		open ($tmpfhname, "<&=$new_fd");
		use strict "refs";
	    }
	    else
	    {
		##  This is a write handle
		no strict "refs";
		open ($tmpfhname, ">&=$new_fd");
		use strict "refs";
	    };
	    (($ok, $msg) = _set_close_on_exec (fileno ($tmpfhname), 0))[0]
	      || &_child_safe_die ("$msg");
	};

	##  Detach if requested
	if ($detach)
	{
	     setpgrp() || &_child_safe_die ("setpgrp() failed: $!");
	};

	##  Set permissions
	if (defined ($gid))
	{
	    ($(, $)) = ($gid, $gid);
	    ((split (/ /, $())[0] == $gid)
		|| &_child_safe_die ("Cannot set rgid to \"$gid\": $!");
	    ((split (/ /, $)))[0] == $gid)
		|| &_child_safe_die ("Cannot set egid to \"$gid\": $!");
	};
	if (defined ($uid))
	{
	    ($<, $>) = ($uid, $uid);
	    ($< == $uid)
		|| &_child_safe_die ("Cannot set ruid to \"$uid\": $!");
	    ($> == $uid)
		|| &_child_safe_die ("Cannot set euid to \"$uid\": $!");
	};

	if (ref ($command[0]))
	{
	    &{$command[0]};
	    return(0, "Should never get here");

	}
	else
	{
	    ##  Exec
	    CORE::exec (@command) ||
		&_child_safe_die ("exec \"", join (" ", @command), "\" failed: $!");
	};
    };

    ##  I am the parent

    ##  Close child filehandles
    foreach $fhandle (@childfh)
    {
	no strict "refs";
	close ($fhandle);
	use strict "refs";
    };

    ##  Register new filehandles,
    ##  but first remove pipes with <use> of "?" or "!",
    ##  and set close-on-exec for those with <use> of "?" and flags of "."
    for ($i = 0; $i < scalar (@$pipes); ++$i)
    {
	if (! length ($use[$i]))
	{
	    push (@pipes_to_register, $pipes->[$i]);
	}
	elsif (($use[$i] eq "?") && ($close_on_exec[$i] eq "."))
	{
	    ##  Set close-on-exec flag
	    $fd = (ref ($fhobj[$i]) ? $fhobj[$i]->fileno()
		   : fileno ($fhobj[$i]));
	    if (! (($ok, $msg) = _set_close_on_exec ($fd, 1))[0])
	    {
		foreach $fhandle (@fhobj)
		{
		    no strict "refs";
		    defined ($fhandle) && close ($fhandle);
		    use strict "refs";
		};
		return ($ok, $msg);
	    };
	};
    };
    if (! (($ok, $msg) = $this->register (@pipes_to_register))[0])
    {
	foreach $fhandle (@fhobj)
	{
	    no strict "refs";
	    defined ($fhandle) && close ($fhandle);
	    use strict "refs";
	};
	return ($ok, $msg);
    };

    ##  Return success
    return ($pid, "");
};

=head2 ->exec (@$pipes, $detach, @command)

Identical to exec_opt, except it does not change uid or gid

=cut

sub exec
{
    my Cam::Scom $this = shift (@_);
    my ($pipes) = shift (@_);
    my ($detach) = shift (@_);
    my (@command) = @_;
    return ($this->exec_opt ($pipes, undef, undef, $detach, @command));
};

=head2 ->connect ($fh, $host, $port, $flags, $nonblocking)


Opens and registers a bidirectional filehandle $fh (must be a
name, not an object) that connects to TCP server $port at
$host.  Both $host and $port may be specified numerically or
by name.  If $host is the null string, the local host is used.
$flags is used as <flags> in register.  If $nonblocking is
true, will perform a nonblocking connect, and failure may only
be detected on subsequent events.  Returns ($ok, $msg) where
$ok is true on success, false on error.  On error, $msg will
be set to an error message.

=cut

sub connect
{
    my Cam::Scom $this = shift (@_);
    my ($fh) = shift (@_);
    my ($remote_host) = shift (@_);
    my ($port) = shift (@_);
    my ($flags) = shift (@_);
    my ($nonblocking) = shift (@_);
    my ($portnum);
    my ($remote_ipaddr, $remote_addr);
    my ($r, $ok, $msg, $fd);

    ##  Get port number
    if (! (($ok, $msg, $portnum) = &_parse_port ($port, "tcp"))[0])
    {
	return ($ok, $msg);
    };

    ##  Get remote host address
    if (! (($ok, $msg, $remote_ipaddr) = &_parse_host ($remote_host))[0])
    {
	return ($ok, $msg);
    };
    $remote_addr = pack_sockaddr_in ($portnum, $remote_ipaddr);

    ##  Open the socket
    defined ($Cam::Scom::Tcp_proto)
	|| defined ($Cam::Scom::Tcp_proto = getprotobyname ("tcp"))
	    || return (0, "Cam::Scom: getprotobyname (\"tcp\") failed: $!");
    no strict "refs";
    if (! socket ($fh, AF_INET, SOCK_STREAM, $Cam::Scom::Tcp_proto))
    {
	return (0, "socket (\"$fh\", AF_INET, SOCK_STREAM, Tcp_proto) failed: $!");
    };
    use strict "refs";

    ##  Get the fd
    $fd = (ref($fh) ? $fh->fileno() : fileno($fh));

    ##  Set non-blocking
    if ($nonblocking)
    {
	if (! (($ok, $msg) = _set_blocking ($fd, 0))[0])
	{
	    return ($ok, $msg);
	};
    };

    ##  Perform the connect
    no strict "refs";
    if (! connect ($fh, $remote_addr))
    {
	$r = $! + 0;
	if (($r != EINPROGRESS) && ($r != EALREADY) && ($r != EINTR))
	{
	    return (0, "connect (\"$fh\", ...) failed: $!");
	};
    };
    use strict "refs";

    ##  Register the connection
    if (! (($ok, $msg) = $this->register ([$fh, "+$flags"]))[0])
    {
	return ($ok, $msg);
    };

    return (1, "");
};

=head2 ->listen ($fh, $port, $rcvname [, $addr]);

Opens and registers a read-only, close-on-exec filehandle $fh
(must be a name, not an object) that receives TCP connections
on port $port, which may be specified numerically or by name.
When new connections are accepted, getevents will return a new
filehandle name of the connection, and the address of the
connected host.  The new filehandle names will be $rcvname
appended by an integer.  The accepted filehandles are not
initially registered, but can be registered with register if desired.
If $addr is specified, listens only on that address (may be a
hostname or "x.x.x.x" format), if not specified, listens on all.
Returns ($ok, $msg) where $msg is true on
success, false on error.  On error, $msg will be set to an
error message.

=cut

sub listen
{
    my Cam::Scom $this = shift (@_);
    my ($fhandle) = shift (@_);
    my ($port) = shift (@_);
    my ($rcvname) = shift (@_);
    my ($addr) = shift (@_);
    my Cam::Scom::Fh $fh;
    my ($name, $aliases, $portnum, $serv_addr, $sockopt_val);
    my ($ok, $msg, $fhmap, $fhname, $fhname_to_index, $fhs);
    my ($listen_addr) = "\0\0\0\0";

    ##  Get port number
    if (! (($ok, $msg, $portnum) = _parse_port ($port, "tcp"))[0])
    {
	return ($ok, $msg);
    };

    ##  Get listen address
    if (defined ($addr))
    {
	$listen_addr = inet_aton ($addr);
    };

    ##  Open the socket
    defined ($Cam::Scom::Tcp_proto)
	|| defined ($Cam::Scom::Tcp_proto = getprotobyname ("tcp"))
	    || return (0, "Cam::Scom: getprotobyname (\"tcp\") failed: $!");
    no strict "refs";
    if (! socket ($fhandle, AF_INET, SOCK_STREAM, $Cam::Scom::Tcp_proto))
    {
	return (0, "socket (\"$fhandle\", AF_INET, SOCK_STREAM, Tcp_proto) failed: $!");
    };
    use strict "refs";

    ##  Don't wait for any previous use to timeout
    $sockopt_val = pack ("i", 1);
    no strict "refs";
    if (! defined ($_ = (setsockopt($fhandle, SOL_SOCKET,
				    SO_REUSEADDR, $sockopt_val))))
    {
	return (0, "setsockopt (\"$fhandle\", SOL_SOCKET, SO_REUSEADDR, 1) failed: $!");
    };
    use strict "refs";

    ##  Bind the socket
    $serv_addr = pack_sockaddr_in ($portnum, $listen_addr);
    no strict "refs";
    if (! bind ($fhandle, $serv_addr))
    {
	return (0, "bind (\"$fhandle\", ...) failed: $!");
    };
    use strict "refs";

    ##  Do the listen
    no strict "refs";
    if (! listen ($fhandle, 5))
    {
	return (0, "listen (\"$fhandle\", 5) failed: $!");
    };
    use strict "refs";

    ##  Register it
    if (! (($ok, $msg) = $this->register ([$fhandle, "<."]))[0])
    {
	return ($ok, $msg);
    };

    ##  And set special registration flags for socket server
    $fhname = _fh_to_name ($fhandle);
    $fhname_to_index = $this->{fhname_to_index};
    $fhs = $this->{fhs};
    $fh = $fhs->[$fhname_to_index->{$fhname}];
    $fh->{read_server} = 1;
    $fh->{client_base} = $rcvname;
    $fh->{read_separator} = "";
    $fh->{separator_len} = 0;

    return (1, "");
};

=head2 ->datagram ($fh, $port, $flags)

Opens and registers a bidirectional filehandle $fhname (name,
not object) of type "*" (messaging) that sends and listens for
datagrams at $port.  $port may be specified numerically or by
name.  $flags is used as <flags> in &ipc'register.  Returns
($ok, $msg) where $ok is true on success, false on error.  On
error, $msg will be set to an error message.

=cut

sub datagram
{
    my Cam::Scom $this = shift (@_);
    my ($fhandle) = shift (@_);
    my ($port) = shift (@_);
    my ($flags) = shift (@_);
    my ($portnum, $serv_addr, $bcast_val, $r, $ok, $msg, $sockopt_val);

    ##  Get port number
    if (! (($ok, $msg, $portnum) = _parse_port ($port, "udp"))[0])
    {
	return ($ok, $msg);
    };

    defined ($Cam::Scom::Udp_proto)
	|| defined ($Cam::Scom::Udp_proto = getprotobyname ("udp"))
	    || return (0, "Cam::Scom: getprotobyname (\"udp\") failed: $!");

    ##  Open the socket
    no strict "refs";
    if (! socket ($fhandle, AF_INET, SOCK_DGRAM, $Cam::Scom::Udp_proto))
    {
	return (0, "socket (\"$fhandle\", AF_INET, SOCK_DGRAM, UDP) failed: $!");
    };
    use strict "refs";

    ##  Don't wait for any previous use to timeout
    $sockopt_val = pack ("i", 1);
    no strict "refs";
    if (! defined ($_ = (setsockopt($fhandle, SOL_SOCKET,
				    SO_REUSEADDR, $sockopt_val))))
    {
	return (0, "setsockopt (\"$fhandle\", SOL_SOCKET, SO_REUSEADDR, 1) failed: $!");
    };
    use strict "refs";

    ##  Allow broadcast
    $bcast_val = pack ("i", 1);
    no strict "refs";
    if (! defined ($_ = (setsockopt ($fhandle, SOL_SOCKET,
				     SO_BROADCAST, $bcast_val))))
    {
	return (0, "setsockopt (\"$fhandle\", SOL_SOCKET, SO_BROADCAST, 1) failed: $!");
    };
    use strict "refs";

    ##  Bind the socket to local port number
    $serv_addr = pack_sockaddr_in ($portnum, "\0\0\0\0");
    no strict "refs";
    if (! bind ($fhandle, $serv_addr))
    {
	return (0, "bind (\"$fhandle\", ...) failed: $!");
    };
    use strict "refs";

    ##  Register the connection
    if (! (($ok, $msg) = $this->register ([$fhandle, "*$flags"]))[0])
    {
	return ($ok, $msg);
    };

    return (1, "");
};

=head2 ->getevents ($timeout [, $filehandle ...])

Reads and writes available data on all registered filehandles,
and looks for events on all registered filehandles specified
in the argument list (no specified filehandles means all
registered filehandles).  It appears to the caller to block
until one of the following occurs:

	    *	$timeout seconds have elapsed with no activity
		on any registered filehandles

	    *	no specified filehandles are readable or writable

	    *   an event was received on a specified filehandle, one of:

	            -	input was received from a specified stream filehandle
			containing its separator

	            -	input was received from a specified stream filehandle
			with a null separator

	            -	input was received from a specified datagram filehandle

	            -	error when reading or writing a specified filehandle

	            -	EOF or close on flush from/to a specified filehandle

A $timeout of -1 is considered infinite.

It returns a list of arrayrefs, or the empty list if no events
have occurred.  Each arrayref points to an array whose first
element is the filehandle being reported in the subsequent
array elements.  Subsequent array elements are either scalars,
in which case they are data received (for streaming
filehandles), or Cam::Scom::Event objects describing an event.
The Cam::Scom::Event objects must be referred to by typed
references (declare as "my Cam::Scom::Event $foo") and have
the following named fields:

	  $event->{fh}      -- the filehandle object being reported
	  $event->{end}     -- true if this is a termination event
	  $event->{term}    -- termination code (if $event->{end}) (see below)
	  $event->{data}    -- the data (for messaging, non-server)(if ! "end")
	  $event->{port}    -- sender port number (for messaging)  (if ! "end")
			       new connect filehandle (for server) (if ! "end")
	  $event->{addr}    -- 4-byte sender addr (for messaging)  (if ! "end")
			       4-byte connect addr (for server)    (if ! "end")

If the "end" field is true for a filehandle, then
close-on-flush, EOF, or error has occurred, the filehandle has
been unregistered, and no more events will be reported for it.
A termination code of 0 indicates either EOF or
close-on-flush.  A positive, non-0 termination code is the
errno reported by the failed read or write.  A negative
termination code has the following meanings:

	    -1  -- the read or write buffer size was exceeded
		   and the filehandle's terminator was turned on.
            -2  -- the read buffer size was exceeded, the filehandle's
		   terminator was turned on, there was a separator
		   defined, and the separator was not detected.
            -3  -- the file descriptor became invalid, and could not
                   be select()'ed

If the file is a stream and the filehandle's separator was
specified as non-null, data elements are broken after each
occurrence of the separator, which is retained in the data.

=cut

sub getevents
{
    my Cam::Scom $this = shift (@_);
    my ($timeout) = shift (@_);
    my (@specd) = @_;
    my Cam::Scom::Fh $fh;
    my Cam::Scom::Event $event;
    my (@result) = ();
    my (@fhs_with_events) = ();
    my ($fh_events, $read_buf);
    my ($ind, $newind, $seplen, $index);
    my ($fhs) = $this->{fhs};
    my ($fhname_to_index) = $this->{fhname_to_index};
    my ($fh_has_event) = $this->{fh_has_event};
    my ($specified) = [];
    my ($events_pending) = 0;

    ##  See if any specified filehandles already have events pending
    if (scalar (@specd))
    {
	##  Make @$specified a cleaned-up list of specified filehandles
	my ($fhandle, $fhname);
	foreach $fhandle (@specd)
	{
	    $fhname = _fh_to_name ($fhandle);
	    if (defined ($index = $fhname_to_index->{$fhname}))
	    {
		push (@$specified, $fhs->[$index]);
		defined ($fh_has_event->{$fhname})
		  && ($events_pending = 1);
	    };
	};
	scalar (@$specified) || return ();
    }
    else
    {
	scalar (@$fhs) || return();
	$events_pending = scalar (keys (%$fh_has_event));
    };

    ##  If there are already events on a specified fh, set timeout to 0
    $events_pending && ($timeout = 0);

    ##  Gather new events
    if (scalar (@$specified))
    {
	$this->_flush (0, $timeout, $specified);
    }
    else
    {
	$this->_flush (0, $timeout);
    };

    ##  Compute list of those specified filehandles that also
    ##  have events pending
    if (scalar (@$specified))
    {
	foreach $fh (@$specified)
	{
	    if (defined ($fh_has_event->{$fh->{fh}}))
	    {
		push (@fhs_with_events, $fh);
	    };
	};
    }
    else
    {
	@fhs_with_events =
	  map ($fhs->[$fhname_to_index->{$_}], keys (%$fh_has_event));
    };

    ##  Return all events
    foreach $fh (@fhs_with_events)
    {
	##  Place all possible events from filehandle $fh into $fh_events
	delete $fh_has_event->{$fh->{fh}};

	##  First, return any data elements
	if (($fh->{messaging}) || ($fh->{read_server}))
	{
	    ##  Messaging handles and socket servers are always
	    ##  already packaged up as events
	    $fh_events = $fh->{read_buf};
	    unshift (@$fh_events, $fh->{handle});
	    $fh->{read_buf} = [];
	    $fh->{read_buf_size} = 0;
	}
	elsif (! $fh->{"separator_len"})
	{
	    ##  Non-separator streams get merged up as a single item
	    $read_buf = $fh->{read_buf};
	    $fh_events = [ $fh->{handle}, join ("", @$read_buf) ];
	    $fh->{read_buf} = [];
	    $fh->{read_buf_size} = 0;
	}
	else
	{
	    ##  Streams with separators must be split into records
	    my ($separator) = $fh->{read_separator};
	    $read_buf = $fh->{read_buf};
	    my ($j) = join ("", @$read_buf);
	    ##  This line breaks with a bug in perl 5.6.1
	    #		$fh_events = [ $fh->{handle}, ($j =~ /(.*?\Q$separator\E)/sg) ];
	    ##  So have to do this instead:
	    $fh_events = [ $fh->{handle} ];
	    $ind = 0;
	    $seplen = length ($separator);
	    while (($newind = index ($j, $separator, $ind)) >= 0)
	    {
		push (@$fh_events,
		      substr ($j, $ind, $newind - $ind + $seplen));
		$ind = $newind + $seplen;
	    };
	    my ($i) = substr
	      ($j, rindex ($j, $separator) + length ($separator));
	    if (length ($i))
	    {
		##  Handle unterminated data at the end (in $i)
		if (! ($fh->{read_open}))
		{
		    push (@$fh_events, $i);
		    $fh->{read_buf} = [];
		    $fh->{read_buf_size} = 0;
		}
		else
		{
		    $fh->{read_buf} = [$i];
		    $fh->{read_buf_size} = length ($i);
		};
	    }
	    else
	    {
		$fh->{read_buf} = [];
		$fh->{read_buf_size} = 0;
	    };
	};

	##  Next, if stopped, push termination event on $fh_events
	if ((! $fh->{read_open}) && (! $fh->{write_open}))
	{
	    my ($handle) = $fh->{handle};
	    $event = new Cam::Scom::Event;
	    $event->{fh} = $handle;
	    $event->{end} = 1;
	    $event->{term} = $fh->{term};
	    push (@$fh_events, $event);
	    ##  This also must unregister the filehandle
	    $this->unregister ($handle);
	}
	##  Finally, see if the select bit needs to be turned back on
	elsif ((! vec ($this->{read_mask}, $fh->{fd}, 1)) &&
	       ($fh->{read_open}))
	{
	    vec ($this->{read_mask}, $fh->{fd}, 1) = 1;
	};

	##  Add this list to @result
	push (@result, $fh_events);
    };

    return (@result);
};

=head2 ->write ($filehandle, $data [, $port, $addr [, $addr ...]])

Writes $data to $filehandle.  $filehandle must have been
previously registered as a ">", "+", or "*" pipe.  If ">" or
"+", not all of the data is guaranteed to be written upon
completion of this call; if the output pipe is full, the data
may be queued for future output by some subsequent
$obj->getevents or $obj->write.  If "*", the message will be
sent immediately to $port at $addr.  A failure will be
returned in this case if the write does not succeed
immediately.  Each $addr can be a dot-notation IP address or a
hostname, or the first and only $addr can be a listref
containing 4-byte IP addresses.  Returns ($ok, $msg) where $ok
is true on success, false on failure.  On failure, $msg is an
error message.

=cut

sub write
{
    my Cam::Scom $this = shift (@_);
    my ($fhandle) = shift (@_);
    my ($data) = shift (@_);
    my Cam::Scom::Fh $fh;
    my ($fhname, $fhname_to_index, $fhs, $index, $write_buf, $datalen,
	$new_buf_size);

    ##  Get the Cam::Scom::Fh object
    $fhname = _fh_to_name ($fhandle);
    $fhname_to_index = $this->{fhname_to_index};
    defined ($index = $fhname_to_index->{$fhname})
      || return (0, "Filehandle \"$fhname\" not registered");
    $fhs = $this->{fhs};
    $fh = $fhs->[$index];

    ##  Is filehandle stopped or close-on-flush?
    if ((! $fh->{write_open}) || ($fh->{close_on_flush}))
    {
	return (0, "Filehandle \"$fhname\" not open for writing");
    };

    if (! $fh->{messaging})
    {
	if ($datalen = length ($data))
	{
	    ##  Buffer it up for the stream
	    $write_buf = $fh->{write_buf};
	    push (@$write_buf, $data);
	    $new_buf_size = ($fh->{write_buf_size} += $datalen);
	    vec ($this->{write_mask}, $fh->{fd}, 1) = 1;
	    if ($new_buf_size > $fh->{write_buf_limit})
	    {
		##  Uh, oh - exceeded the maximum buffer size
		if ($fh->{write_buf_killer})
		{
		    if ((defined ($fh->{write_buf_killmsg})) &&
			length ($fh->{write_buf_killmsg}))
		    {
			##  Replace write buffer with kill msg
			##  and set close-on-flush.
			$fh->{write_buf} = [$fh->{write_buf_killmsg}];
			$fh->{write_buf_size} =
			  length ($fh->{write_buf_killmsg});
			$this->close_on_flush ($fh->{handle});
			##  Remember the termination code
			$fh->{term} = -1;

			##  Try to flush it now
			$this->_flush (0, 0);
		    }
		    else
		    {
			##  Generate an error
			if ($fh->{is_object})
			{
			    $fh->{handle}->close();
			}
			elsif ($fh->{is_fd_only})
			{
			    if (defined ($fh->{fd}))
			    {
				POSIX::close ($fh->{fd});
			    };
			}
			else
			{
			    no strict "refs";
			    close ($fh->{handle});
			    use strict "refs";
			};
			$this->_stop_pipe ($fh, -1);
		    };
		}
		else
		{
		    ##  Have to just do writes until this
		    ##  write buffer is flushed
		    $this->_flush (1, -1, [$fh]);
		}
	    }
	    else
	    {
		##  Try to flush it now
		$this->_flush (0, 0);
	    };
	};
    }
    else
    {
	##  Send a message immediate
	my ($port) = shift (@_);
	my (@addrs) = @_;
	my (@ip_addrs, $ok, $msg, $portnum, $r, $ip_addr);
	if (! defined ($port))
	{
	    return (0, "port argument not present for datagram send");
	};
	if (! (($ok, $msg, $portnum) = &_parse_port ($port, "udp"))[0])
	{
	    return ($ok, $msg);
	};
	if (! scalar (@addrs))
	{
	    return (0, "address argument not present for datagram send");
	};
	##  Normal datagram
	if (ref ($addrs[0]))
	{
	    $ip_addr = $addrs[0];
	    @addrs = @$ip_addr;
	}
	else
	{
	    foreach (@addrs)
	    {
		if (! (($ok, $msg, $_) = &_parse_host ($_))[0])
		{
		    return ($ok, $msg);
		};
	    };
	};
	foreach $ip_addr (@addrs)
	{
	    my ($addr_buf) = pack_sockaddr_in ($port, $ip_addr);
	    if ($fh->{is_object})
	    {
		$fhandle->send ($data, 0, $addr_buf);
	    }
	    else
	    {
		no strict "refs";
		$r = send ($fhandle, $data, 0, $addr_buf);
		use strict "refs";
	    };
	    if (! defined ($r))
	    {
		$addr_buf =
		  join (".", unpack ("CCCC",
				     (unpack_sockaddr_in ($ip_addr))[1]));
		return (0, "sendto (\"$fhandle\", ..., port = $port, addr = $addr_buf) failed: $!");
	    };
	};
    };

    return (1, "");
};

=head2 ->expect ($receive_pipe, $to_receive, $timeout)

Blocks until  the entire   input on   registered $receive_pipe
matches regular expression  $to_receive, or no activity occurs
on the pipe for $timeout  seconds, or $receive_pipe closes.  A
timeout of -1 is infinite.  The  regular expression is in Perl
format, with delimiters.

This is probably only meaningful on stream connections.

Returns an array of five elements:

	    (-match-, -input-, -pipe_up-, -pipe_code-, -syntax-)

		-match-	    ::=	1 if the whole input matched
				$to_receive, 0 o.w.
		-input-     ::=	The input received
		-pipe_up-   ::=	1 if $receive_pipe is still open, 0 o.w.
		-pipe_code- ::=	The pipe termination code if -pipe_up- == 0
		-syntax-    ::=	"" if no syntax error in the regular
				expression $to_receive, a Perl error
				message o.w.


=cut

sub expect
{
    my Cam::Scom $this = shift (@_);
    my ($receive_pipe) = shift (@_);
    my ($to_receive) = shift (@_);
    my ($timeout) = shift (@_);
    my ($match, $datum, $datumlist, @events);
    my Cam::Scom::Event $next_event;
    my ($pipe_up, $pipe_code) = (1, 0);
    my ($input_so_far) = "";

    while (1)
    {
	scalar (@events = $this->getevents ($timeout, $receive_pipe))
	  || return (0, $input_so_far, 1, 0, "");
	while (defined ($datumlist = shift (@events)))
	{
	    shift (@$datumlist);
	    while (defined ($datum = shift (@$datumlist)))
	    {
		if (ref ($datum))
		{
		    $next_event = $datum;
		    ($pipe_up, $pipe_code) = (0, $next_event->{term});
		}
		else
		{
		    $input_so_far .= $datum;
		};
		$match = eval ("\$input_so_far =~ $to_receive");
		if ($match || (length ($@)) || (! $pipe_up))
		{
		    return ($match, $input_so_far, $pipe_up, $pipe_code, $@);
		};
	    };
	};
    };
};

=head2 ->close_on_flush ($filehandle)

Sets the registered writable filehandle to close and terminate
after all previously written data has been output.  Note that,
for socket connections, even outputing the data does not
guarantee that it will be delivered before the close (unless
SO_LINGER has been set).  Subsequent attempts to write to this
filehandle will fail.  Returns 1 on success, 0 if the
filehandle is not currently registered.  A filehandle termination
event will be sent on filehandle close.

=cut

sub close_on_flush
{
    my Cam::Scom $this = shift (@_);
    my ($fhandle) = shift (@_);
    my Cam::Scom::Fh $fh;
    my ($fhs, $index, $fhname, $fhname_to_index);

    ##  Is filehandle unregistered?
    $fhname = _fh_to_name ($fhandle);
    $fhname_to_index = $this->{fhname_to_index};
    (! defined ($index = $fhname_to_index->{$fhname}))
      && return (0, "Filehandle \"$fhname\" not registered");
    $fhs = $this->{fhs};
    $fh = $fhs->[$index];

    ##  Is filehandle already stopped?
    (! $fh->{write_open}) && (return (1));

    ##  Is filehandle already flushed?
    if (! $fh->{write_buf_size})
    {
	##  If so, stop it and close it.
	if ($fh->{is_object})
	{
	    $fh->{handle}->close();
	}
	elsif ($fh->{is_fd_only})
	{
	    if (defined ($fh->{fd}))
	    {
		POSIX::close ($fh->{fd});
	    };
	}
	else
	{
	    no strict "refs";
	    close ($fh->{handle});
	    use strict "refs";
	};
	$this->_stop_pipe ($fh, 0);
	return (1);
    };

    ##  Set close_on_flush
    $fh->{close_on_flush} = 1;
    return (1);
};

=head2 ->set_read_buf_limit ($handle, $size)

Set the read & write buffer size limits & terminators.  These
change the handling of buffer full conditions to the following
cases:

	  Read buffer exceeds limit, no terminator, event(s) pending:

	    Flow control backpressure is exerted by not doing more
	    reads on that handle until events are retrieved from that
	    handle with getevents().

	  Read buffer exceeds limit, no terminator, no events pending:

	    Handle is closed and a termination event is posted.

	  Read buffer exceeds limit, terminator specified:

	    Handle is closed and a termination event is posted.

	  Write buffer exceeds limit, no terminator:

	    No reads will be done on any registered handle (thus
	    exerting flow control backpressure on *all* registered
	    read handles) until the write buffer is completely cleared
	    for that handle.

	  Write buffer exceeds limit, terminator specified, no stop_msg:

	    Handle is closed and a termination event is returned.

	  Write buffer exceeds limit, terminator specified, stop_msg specified:

	    The write buffer is cleared and replaced by the contents
	    of $stop_msg, and the handle is set to close_on_flush.

When a handle is first registered its buffer limit size(s) are
set to 1MB and its terminator(s) are unspecified.

=cut

sub set_read_buf_limit
{
    my Cam::Scom $this = shift (@_);
    my ($fhandle) = shift (@_);
    my ($size) = shift (@_);
    my Cam::Scom::Fh $fh;
    my ($index, $fhname, $fhname_to_index, $fhs);

    ##  Is filehandle unregistered?
    $fhname = _fh_to_name ($fhandle);
    $fhname_to_index = $this ->{fhname_to_index};
    (! defined ($index = $fhname_to_index->{$fhname}))
      && return (0, "Filehandle \"$fhname\" not registered");
    $fhs = $this->{fhs};
    $fh = $fhs->[$index];

    ##  Is filehandle readable?
    $fh->{read_open}
    || return (0, "Filehandle \"$fhname\" not open for reading");

    ##  Set buf limit
    $fh->{read_buf_limit} = $size;

    return (1);
};

=head2 ->set_read_buf_limit_terminator ($handle, $do_terminate)

See set_read_buf_limit.

=cut

sub set_read_buf_limit_terminator
{
    my Cam::Scom $this = shift (@_);
    my ($fhandle) = shift (@_);
    my ($do_terminator) = shift (@_);
    my Cam::Scom::Fh $fh;
    my ($index, $fhname, $fhname_to_index, $fhs);

    ##  Is filehandle unregistered?
    $fhname = _fh_to_name ($fhandle);
    $fhname_to_index = $this->{fhname_to_index};
    (! defined ($index = $fhname_to_index->{$fhname}))
      && return (0, "Filehandle \"$fhname\" not registered");
    $fhs = $this->{fhs};
    $fh = $fhs->[$index];

    ##  Is filehandle readable?
    $fh->{read_open}
    || return (0, "Filehandle \"$fhname\" not open for reading");

    ##  Set terminator flag
    $fh->{read_buf_killer} = $do_terminator;

    return (1);
};

=head2 ->set_write_buf_limit ($handle, $size)

See set_read_buf_limit.

=cut

sub set_write_buf_limit
{
    my Cam::Scom $this = shift (@_);
    my ($fhandle) = shift (@_);
    my ($size) = shift (@_);
    my Cam::Scom::Fh $fh;
    my ($index, $fhs, $fhname, $fhname_to_index);

    ##  Is filehandle unregistered?
    $fhname = _fh_to_name ($fhandle);
    $fhname_to_index = $this->{fhname_to_index};
    (! defined ($index = $fhname_to_index->{$fhname}))
      && return (0, "Filehandle \"$fhname\" not registered");
    $fhs = $this->{fhs};
    $fh = $fhs->[$index];

    ##  Is filehandle writable?
    $fh->{write_open}
    || return (0, "Filehandle \"$fhname\" not open for writing");

    ##  Set buf limit
    $fh->{write_buf_limit} = $size;

    return (1);
};

=head2 ->set_write_buf_limit_terminator ($handle, $do_terminate, [$stop_msg])

See set_read_buf_limit.

=cut

sub set_write_buf_limit_terminator
{
    my Cam::Scom $this = shift (@_);
    my ($fhandle) = shift (@_);
    my ($do_terminator) = shift (@_);
    my ($stop_msg) = shift (@_);
    my Cam::Scom::Fh $fh;
    my ($index, $fhname, $fhname_to_index, $fhs);

    ##  Is filehandle unregistered?
    $fhname = _fh_to_name ($fhandle);
    $fhname_to_index = $this->{fhname_to_index};
    (! defined ($index = $fhname_to_index->{$fhname}))
      && return (0, "Filehandle \"$fhname\" not registered");
    $fhs = $this->{fhs};
    $fh = $fhs->[$index];

    ##  Is filehandle writable?
    $fh->{write_open}
    || return (0, "Filehandle \"$fhname\" not open for writing");

    ##  Set terminator flag
    $fh->{write_buf_killer} = $do_terminator;
    if ($do_terminator && defined ($stop_msg) && length ($stop_msg))
    {
	$fh->{write_buf_killmsg} = $stop_msg;
    }
    else
    {
	$fh->{write_buf_killmsg} = undef;
    };

    return (1);
};

=head2 ->readers()

Returns a list of filehandles currently registered for
reading.

=cut

sub readers
{
    my Cam::Scom $this = shift (@_);
    my Cam::Scom::Fh $fh;
    my (@results) = ();
    my ($fhs);

    $fhs = $this->{fhs};
    foreach $fh (@$fhs)
    {
	$fh->{read_open} && (push (@results, $fh->{handle}));
    };
    return (@results);
};

=head2 ->writers()

Returns a list of filehandles currently registered for
writing.

=cut

sub writers
{
    my Cam::Scom $this = shift (@_);
    my Cam::Scom::Fh $fh;
    my (@results) = ();
    my ($fhs);

    $fhs = $this->{fhs};
    foreach $fh (@$fhs)
    {
	$fh->{write_open} && (push (@results, $fh->{handle}));
    };
    return (@results);
};

=head2 ->write_blocked()

Returns a list of filehandles (either strings or FileHandles)
currently blocked on output.

=cut

sub write_blocked
{
    my Cam::Scom $this = shift (@_);
    my Cam::Scom::Fh $fh;
    my (@results);
    foreach $fh (@{$this->{fhs}})
    {
	if ($fh->{write_buf_size})
	{
	    push (@results, $fh->{handle});
	};
    };
    return (@results);
};

=head2 ->write_blocked_size()

Returns a hashref mapping filehandle names (always converted
to strings) currently blocked on output to the number of bytes
awaiting output.

=cut

sub write_blocked_size
{
    my Cam::Scom $this = shift (@_);
    my Cam::Scom::Fh $fh;
    my ($results);
    foreach $fh (@{$this->{fhs}})
    {
	if ($fh->{write_buf_size})
	{
	    $results->{$fh->{fh}} = $fh->{write_buf_size};
	};
    };
    return ($results);
};

=head2 c_socket(domain,type,protocol)

Returns ($ok, $msg, $fd) where $fd is the file descriptor
obtained from the system call socket(2).
$ok is true on success.  $msg is an error message on failure,
in which case $fd will be undef.

=cut

sub c_socket
{
    my ($domain) = shift;
    my ($type) = shift;
    my ($protocol) = shift;
    my ($fd);

    $fd = Cam::Scom::_c_socket ($domain, $type, $protocol);
    ($fd == -1) && return (0, "$!");
    return (1, "", $fd);
};

=head2 c_socketpair(domain,type,protocol)

Returns ($ok, $msg, $fd0, $fd1) where $fd0 and $fd1 are the file descriptors
obtained from the system call socketpair(2).
$ok is true on success.  $msg is an error message on failure,
in which case $fd0 and $fd1 will be undef.

=cut

sub c_socketpair
{
    my ($domain) = shift;
    my ($type) = shift;
    my ($protocol) = shift;
    my ($r, $fd0, $fd1);

    ($r, $fd0, $fd1) = Cam::Scom::_c_socketpair ($domain, $type, $protocol);
    ($r == -1) && return (0, "$!");
    return (1, "", $fd0, $fd1);
};

=head2 c_getsockopt(fd,level,optname)

Returns ($ok, $msg, $value) where $value is the result
obtained from the system call getsockopt(2).
$ok is true on success.  $msg is an error message on failure,
in which case $value will be undef.

=cut

sub c_getsockopt
{
    my ($fd) = shift;
    my ($level) = shift;
    my ($optname) = shift;
    my ($r, $value);

    ($r, $value) = Cam::Scom::_c_getsockopt ($fd, $level, $optname);
    ($r == -1) && return (0, "$!");
    return (1, "", $value);
};

=head2 c_setsockopt(fd,level,optname)

Returns ($ok, $msg) from the system call setsockopt(2).
$ok is true on success.  $msg is an error message on failure.

=cut

sub c_setsockopt
{
    my ($fd) = shift;
    my ($level) = shift;
    my ($optname) = shift;
    my ($r);

    $r = Cam::Scom::_c_setsockopt ($fd, $level, $optname);
    ($r == -1) && return (0, "$!");
    return (1, "");
};

=head2 c_bind(fd,addr)

Returns ($ok, $msg) from the system call bind(2).
$ok is true on success.  $msg is an error message on failure.

=cut

sub c_bind
{
    my ($fd) = shift;
    my ($addr) = shift;
    my ($r);

    $r = Cam::Scom::_c_bind ($fd, $addr);
    ($r == -1) && return (0, "$!");
    return (1, "");
};

=head2 c_listen(fd,backlog)

Returns ($ok, $msg) from the system call listen(2).
$ok is true on success.  $msg is an error message on failure.

=cut

sub c_listen
{
    my ($fd) = shift;
    my ($backlog) = shift;
    my ($r);

    $r = Cam::Scom::_c_listen ($fd, $backlog);
    ($r == -1) && return (0, "$!");
    return (1, "");
};

=head2 c_accept(fd)

Returns ($ok, $msg, $new_fd, $addr) from the system call accept(2).
$ok is true on success.  $msg is an error message on failure.
$new_fd is the new file descriptor accepted, and $addr is the
struct sockaddr encoding its address.

=cut

sub c_accept
{
    my ($fd) = shift;
    my ($new_fd, $addr);

    ($new_fd, $addr) = Cam::Scom::_c_accept ($fd);
    ($new_fd == -1) && return (0, "$!");
    return (1, "", $new_fd, $addr);
};

=head2 c_connect(fd,addr)

Returns ($ok, $msg) from the system call connect(2).
$ok is true on success.  $msg is an error message on failure.
$addr is the struct sockaddr encoding the address to connect to.

=cut

sub c_connect
{
    my ($fd) = shift;
    my ($addr) = shift;
    my ($r);

    $r = Cam::Scom::_c_connect ($fd, $addr);
    ($r == -1) && return (0, "$!");
    return (1, "");
};

sub _child_safe_die
{
    my (@msg) = @_;
    my ($debugmsg) =
      "Cam::Scom pre-exec child error : " . join ("", @msg) . "\n";
    POSIX::write (2, $debugmsg, length ($debugmsg));
    POSIX::_exit (1);
};


##  &DESTROY
##    Destroys a Cam::Scom object
sub DESTROY
{
};

sub _do_execute
{
    my ($uid) = shift (@_);
    my ($gid) = shift (@_);
    my ($detach) = shift (@_);
    my ($string_stdin) = shift (@_);
    my ($input) = shift (@_);
    my (@command) = @_;
    my ($scom) = new Cam::Scom;
    my ($cmd_stdout) = Private_fh_prefix . "SOUT";
    my ($cmd_stdin) = Private_fh_prefix . "SIN";
    my ($cmd_stderr) = Private_fh_prefix . "SERR";
    my Cam::Scom::Event $event;
    my (@parray, $ok, $msg, $oldsigchld, $datum);
    my ($pid, @events, @outbuf, @errbuf, $fh, $data, $code, $datumlist);

    @parray = ("$cmd_stdout<1:", "$cmd_stderr<2:");
    if ($string_stdin)
    {
	push (@parray, "$cmd_stdin>0");
    };
    ##  Have to prevent the termination status being reaped by someone else
    if ((defined ($oldsigchld = $::SIG{"CHLD"})) &&
	($oldsigchld ne "DEFAULT"))
    {
	$::SIG{"CHLD"} = "DEFAULT";
    }
    else
    {
	undef $oldsigchld;
    };
    if (! (($pid, $msg) =
	   $scom->exec_opt (\@parray, $uid, $gid, $detach, @command))[0])
    {
	(defined ($oldsigchld)) && ($::SIG{"CHLD"} = $oldsigchld);
	return (0, $msg);
    };
    if ($string_stdin)
    {
	if (! (($ok, $msg) = $scom->write ($cmd_stdin, $input))[0])
	{
	    (defined ($oldsigchld)) && ($::SIG{"CHLD"} = $oldsigchld);
	    return ($ok, $msg);
	};
	if (! $scom->close_on_flush ($cmd_stdin))
	{
	    (defined ($oldsigchld)) && ($::SIG{"CHLD"} = $oldsigchld);
	    return
		(0, "Internal error: close_on_flush ($cmd_stdin) failed");
	};
    };
    while (scalar (@events = $scom->getevents(-1)))
    {
	while (defined ($datumlist = shift (@events)))
	{
	    $fh = shift (@$datumlist);
	    while (defined ($datum = shift (@$datumlist)))
	    {
		if ($fh eq $cmd_stdin)
		{
		    if (! ref ($datum))
		    {
			(defined ($oldsigchld)) &&
			  ($::SIG{"CHLD"} = $oldsigchld);
			return (0, "comevent reports data for $cmd_stdin");
		    }
		    else
		    {
			$event = $datum;
			if (($code = $event->{term}) != 0)
			{
			    (defined ($oldsigchld)) &&
			      ($::SIG{"CHLD"} = $oldsigchld);
			    return (0, "Error writing to stdin, errno = $code");
			};
		    };
		}
		elsif ($fh eq $cmd_stdout)
		{
		    if (ref ($datum))
		    {
			$event = $datum;
			if (($code = $event->{term}) != 0)
			{
			    (defined ($oldsigchld)) &&
			      ($::SIG{"CHLD"} = $oldsigchld);
			    return (0, "Error reading stdout, errno = $code");
			};
		    }
		    else
		    {
			push (@outbuf, $datum);
		    };
		}
		elsif ($fh eq $cmd_stderr)
		{
		    if (ref ($datum))
		    {
			$event = $datum;
			if (($code = $event->{term}) != 0)
			{
			    (defined ($oldsigchld)) &&
			      ($::SIG{"CHLD"} = $oldsigchld);
			    return (0, "Error reading stderr, errno = $code");
			};
		    }
		    else
		    {
			push (@errbuf, $datum);
		    };
		}
		else
		{
		    (defined ($oldsigchld)) && ($::SIG{"CHLD"} = $oldsigchld);
		    return (0, "comevent returned bad filehandle: \"$fh\"");
		};
	    };
	};
    };

    if (waitpid ($pid, 0) == -1)
    {
	(defined ($oldsigchld)) && ($::SIG{"CHLD"} = $oldsigchld);
	return (0, "waitpid ($pid, 0) returned -1: $!");
    };
    (defined ($oldsigchld)) && ($::SIG{"CHLD"} = $oldsigchld);
    return (1, "", join ("", @outbuf), join ("", @errbuf));
};

##  _fh_to_name ($spec)
##    Returns $name which is the unique name of $spec,
##    the thing that can be passed to "fhname_to_index"
sub _fh_to_name
{
    my ($fh) = shift (@_);
    if (ref ($fh) eq "FileHandle")
    {
	return ("$fh");
    }
    elsif ($fh =~ /^\d+$/)
    {
	return ("fd=$fh");
    }
    else
    {
	return ($fh);
    };
};

##  &_flush ($write_only, $timeout, \@filehandles) will continue processing
##  I/O on all registered filehandles until one of the following occurs:
##      *  no activity has occurred for $timeout on any registered filehandles
##      *  no specified filehandle is readable/writable
##      *  a new event occured on a specified filehandle
##  @filehandles is the list of Cam::Scom::Fh objects.  An empty list
##  means all registered filehandles are specified.  A $timeout of -1 is
##  infinite.  It will never return until all immediately processable I/O for
##  all registered filehandles has been processed (except those for which
##  the full buffer has already been read).  If $write_only is specified,
##  no reads are done, only writes are processed.
sub _flush
{
    my Cam::Scom $this = shift (@_);
    my ($write_only) = shift (@_);
    my ($timeout) = shift (@_);
    my ($specified) = shift (@_);
    my Cam::Scom::Fh $fh;
    my Cam::Scom::Event $datum;
    my ($reads_requested, $writes_requested, $errors_requested,
	$reads, $writes, $errors, $index, $read_buf, $fd,
	$activity, $buf, $r, $list, $errno, $messaging, $spec_select);
    my ($fd_to_fh, $reads_returned, $writes_returned, $spec_mask);
    my ($fhs) = $this->{fhs};
    my ($fh_has_event);

    defined ($timeout) && ($timeout == -1) && ($timeout = undef);
    $fd_to_fh = $this->{fd_to_fh};
    $fh_has_event = $this->{fh_has_event};

    ##  Set the $spec_mask to include all specified filehandles.
    ##  If this and'd with select mask is 0, we're done.
    if (! defined ($specified))
    {
	$specified = $fhs;
	$spec_mask = $this->{all_mask};
    }
    else
    {
	$spec_mask = $this->{empty_mask};
	foreach $fh (@$specified)
	{
	    vec ($spec_mask, $fh->{fd}, 1) = 1;
	};
    };

    ##  DEBUG
##    print ::STDOUT ("_flush entered\n");

    ##  $activity is whether anything happened on any filehandle
    ##  the last time around.
    $activity = 1;
    if ($write_only)
    {
	$reads_requested = \ $this->{empty_mask}
    }
    else
    {
	$reads_requested = \ $this->{read_mask};
    };
    $writes_requested = \ $this->{write_mask};
    $errors_requested = \ $this->{error_mask};
    while ($activity &&
	   ((($spec_mask & $$reads_requested) ne $this->{empty_mask}) ||
	    (($spec_mask & $$writes_requested) ne $this->{empty_mask})))
    {
	$activity = 0;

	##  DEBUG
##	print ::STDOUT ("\tDoing select on reads = ",
##			join (" ", @{$this->_masks_to_array
##			      ($$reads_requested, $$errors_requested)}),
##			", writes = ",
##			join (" ", @{$this->_masks_to_array
##			      ($$writes_requested, $$errors_requested)}),
##			", specified = ",
##			join (" ", @{$this->_masks_to_array
##				       ($spec_mask, $spec_mask)}),
##			"\n");

	##  Do the select
	$r = select ($reads = $$reads_requested,
		     $writes = $$writes_requested,
		     $errors = $$errors_requested, $timeout);
	if ($r == -1)
	{
	    $errno = $! + 0;
	    if ($errno == EBADF)
	    {
		##  One of the file descriptors was bad
		if (defined ($fh = $this->_find_bad_fh()))
		{
		    if (vec ($spec_mask, $fh->{fd}, 1))
		    {
			$timeout = 0;
		    };
		    $this->_stop_pipe ($fh, -3);
		}
		else
		{
		    die ("Cam::Scom::_flush(): error from select: EBADF but no bad descriptors found\n");
		};
	    }
	    elsif ($errno != EINTR)
	    {
		$! = $errno;
		die ("Cam::Scom::_flush(): error from select: $!\n")
	    };
	    $activity = 1;
	    next;
	}
	elsif ($r == 0)
	{
	    next;
	};

	##  Process all ready readable fds
	if (! $write_only)
	{
	    $reads_returned = $this->_masks_to_array ($reads, $errors);

	    ##  DEBUG
##	    print STDOUT ("Found readers = ",
##	    join (" ", @$reads_returned), "\n");

	    foreach $fd (@$reads_returned)
	    {
		$fh = $fd_to_fh->[$fd];
		$activity = 1;
		$read_buf = $fh->{read_buf};

		##  DEBUG
##		print STDOUT
##		  ("Reading from fd=" . $fd . " (" . $fh->{fh} . ")\n");

		##  See if it is a listening socket or not
		if (! $fh->{read_server})
		{
		    ##  Not a listening socket.  Perform the input,
		    ##  set $r to data length returned, or -1 if error.
		    ##  if $r > 0, set $datum to the event of the input
		    ##  info if it's a datagram, o.w. just leave in $buf
		    ##  if it's stream data.  If $r == -1, set $errno.
		    if ($messaging = $fh->{messaging})
		    {
			my ($from_buf);

		        ##  A messaging handle
			if ($fh->{is_object})
			{
			    $from_buf = $fh->{handle}->recv
			      ($buf, Blocksize, 0);
			}
			else
			{
			    no strict "refs";
			    $from_buf = recv
			      ($fh->{handle}, $buf, Blocksize, 0);
			    use strict "refs";
			};
			if (defined ($from_buf))
			{
			    $r = length ($buf);
			}
			else
			{
			    $r = -1;
			};
			if ($r > 0)
			{
			    $datum = new Cam::Scom::Event;
			    $datum->{fh} = $fh->{handle};
			    $datum->{end} = 0;
			    ($datum->{port}, $datum->{addr}) =
			      unpack_sockaddr_in ($from_buf);
			    $datum->{data} = $buf;
			};
		    }
		    else
		    {
			##  A streaming handle
			$r = POSIX::read ($fh->{fd}, $buf, Blocksize);
			defined ($r) || ($r = -1);
		    };

		    ##  Now $r, $datum, $buf, and $errno are set.
		    if (($r == -1) && ($errno = $! + 0) &&
			(($errno == EAGAIN) || ($errno == EINTR)))
		    {
			##  Ignore
		    }
		    elsif ($r <= 0)
		    {
			if ($r == -1)
			{
			    ##  Pipe died
			    $errno = $! + 0;
			}
			elsif ($r == 0)
			{
			    ##  EOF
			    $errno = 0;
			}
			if (vec ($spec_mask, $fh->{fd}, 1))
			{
			    $timeout = 0;
			};
			$this->_stop_pipe ($fh, $errno);
		    }
		    else
		    {
			##  A successful read of $r bytes from filehandle $fh.

			##  Log it.
			if (defined ($this->{log_dir}))
			{
			    my ($fhname) = $fh->{fh};
			    my ($log_dir) = $this->{log_dir};
			    if (! defined ($fh->{read_logfile}))
			    {
				my ($fname) = "$log_dir/READ.$fhname";
				no strict "refs";
				open ("READLOG_$fhname", ">$fname")
				    || (print STDERR
					("Cam::Scom log: Cannot open \"",
					 $fname,
					 "\" for writing: $!"));
				select
				  ((select ("READLOG_$fhname"), $| = 1)[0]);
				use strict "refs";
				$fh->{read_logfile} = "READLOG_$fhname";
			    };
			    no strict "refs";
			    if ($fh->{messaging})
			    {
				my ($msg) =
				  join ("",
					"\n",
					join (".", unpack
					      ("C4", $datum->{addr})),
					":",
					$datum->{port},
					"\n");
				(syswrite ($fh->{read_logfile},
					   $msg, length($msg)))
				    || (print STDERR
					("Cam::Scom log: Failed write to ",
					 $fh->{read_logfile},
					 ": $!"));
			    };
			    (syswrite ($fh->{read_logfile}, $buf, $r))
				|| (print STDERR
				    ("Cam::Scom log: Failed write to ",
				     $fh->{read_logfile},
				     ": $!"));
			    use strict "refs";
			};

			##  Add to the read_buf, see if
			##  merge with previous data is necessary,
			##  and check if this is a new event.
			if ($messaging)
			{
			    push (@$read_buf, $datum);
			    ##  Always a new event if none yet
			    if (! defined ($fh_has_event->{$fh->{fh}}))
			    {
				$fh_has_event->{$fh->{fh}} = 1;
				vec ($spec_mask, $fh->{fd}, 1)
				  && ($timeout = 0);
			    };
			}
			elsif (($fh->{separator_len} > 1) &&
			       ($index = scalar (@$read_buf)))
			{
			    ##  Append the data and check for separator
			    --$index;
			    $read_buf->[$index] .= $buf;
			    if ((! defined ($fh_has_event->{$fh->{fh}})) &&
				(index ($read_buf->[$index],
					$fh->{read_separator}) != -1))
			    {
				$fh_has_event->{$fh->{fh}} = 1;
				vec ($spec_mask, $fh->{fd}, 1)
				  && ($timeout = 0);
			    };
			}
			else
			{
			    ##  Just add to the list and check for separator
			    push (@$read_buf, $buf);
			    if ((! defined ($fh_has_event->{$fh->{fh}})) &&
				((! $fh->{separator_len}) ||
				 (index ($buf, $fh->{read_separator}) != -1)))
			    {
				$fh_has_event->{$fh->{fh}} = 1;
				vec ($spec_mask, $fh->{fd}, 1)
				  && ($timeout = 0);
			    };
			};

			##  Increment the total read buf size
			if (($fh->{read_buf_size} += $r) >
			    $fh->{read_buf_limit})
			{
			    ##  Uh, oh - the max read buffer size has
			    ##  been exceeded.
			    if ((! $fh->{read_buf_killer}) &&
				defined ($fh_has_event->{$fh->{fh}}))
			    {
				##  Turn off the fd's read select bit
				##  to stop reading until events are
				##  cleared out
				$fd = $fh->{fd};
				vec ($$reads_requested, $fd, 1) = 0;
				if (! vec ($$writes_requested, $fd, 1))
				{
				    vec ($$errors_requested, $fd, 1) = 0;
				};
			    }
			    else
			    {
				##  Kill this filehandle
				if ($fh->{read_buf_killer})
				{
				    ##  terminator ==> kill this handle
				    $errno = -1;
				}
				else
				{
				    ##  No event after max read size is error
				    $errno = -2;
				};
				if ($fh->{is_object})
				{
				    $fh->{handle}->close();
				}
				elsif ($fh->{is_fd_only})
				{
				    if (defined ($fh->{fd}))
				    {
					POSIX::close ($fh->{fd});
				    };
				}
				else
				{
				    no strict "refs";
				    close ($fh->{handle});
				    use strict "refs";
				};
				if (vec ($spec_mask, $fh->{fd}, 1))
				{
				    $timeout = 0;
				};
				$this->_stop_pipe ($fh, $errno);
			    };
			};
		    };
		}
		else
		{
		    ##  It is a listening socket.
		    my ($newfh, $addr);
		    if ($fh->{is_object})
		    {
			($newfh, $addr) = $fh->{handle}->accept();
		    }
		    else
		    {
			$newfh = $fh->{client_base} . $fh->{read_server}++;
			no strict "refs";
			$addr = accept ($newfh, $fh->{handle});
			use strict "refs";
		    };
		    if (! defined ($addr))
		    {
			$errno = $! + 0;
			if (($errno != EAGAIN) && ($errno != EINTR))
			{
			    ##  Listener died
			    if (vec ($spec_mask, $fh->{fd}, 1))
			    {
				$timeout = 0;
			    };
			    $this->_stop_pipe ($fh, $errno);
			};
		    }
		    else
		    {
			##  Got a new filehandle,
			##  put it in the buffer to be returned.
			$datum = new Cam::Scom::Event;
			$datum->{fh} = $fh->{handle};
			$datum->{end} = 0;
			($_, $datum->{addr}) = unpack_sockaddr_in ($addr);
			$datum->{"port"} = $newfh;
			push (@$read_buf, $datum);
			$fh_has_event->{$fh->{fh}} = 1;
			vec ($spec_mask, $fh->{fd}, 1) && ($timeout = 0);
		    };
		};
	    };
	};

	##  Process all ready writable fds
	$writes_returned = $this->_masks_to_array ($writes, $errors);

	##  DEBUG
##	print STDOUT ("\nFound writers = ",
##                    join (" ", @$writes_returned), "\n");

	foreach $fd (@$writes_returned)
	{
	    ##  Have to watch out here because the fd may
	    ##  have already been deleted in the read loop above
	    if (defined ($fh = $fd_to_fh->[$fd]))
	    {
		##  Stuff to write
		$activity = 1;

		##  DEBUG
##		print STDOUT
##		  ("Writing to fd=" . $fd . " (" . $fh->{fh} . ")\n");

		##  Bunch up small writes into larger blocks
		$list = $fh->{write_buf};
		$index = $fh->{write_buf_size};
		$buf = shift (@$list);
		while ((($r = length ($buf)) < Blocksize) &&
		       ($r < $index))
		{
		    $buf .= shift (@$list);
		};
		$r = POSIX::write ($fh->{fd}, $buf, length ($buf));
		if (! defined ($r))
		{
		    ##  Pipe got an error
		    $errno = $! + 0;
		    if (($errno != EAGAIN) && ($errno != EINTR))
		    {
			if (vec ($spec_mask, $fh->{fd}, 1))
			{
			    $timeout = 0;
			};
			$this->_stop_pipe ($fh, $errno);
		    };
		}
		else
		{
		    ##  A successful write of $r bytes to filehandle $fh.
		    $fh->{write_buf_size} -= $r;

		    ##  Log it.
		    if (defined ($this->{log_dir}) && $r)
		    {
			my ($fhname) = $fh->{fh};
			my ($log_dir) = $this->{log_dir};
			if (! defined ($fh->{write_logfile}))
			{
			    my ($fname) = "$log_dir/WRITE.$fhname";
			    no strict "refs";
			    open ("WRITELOG_$fhname", ">$fname")
			      || (print STDERR
				  ("Cam::Scom log: Cannot open \"",
				   $fname,
				   "\" for writing : $!"));
			    select ((select ("WRITELOG_$fhname"), $| = 1)[0]);
			    use strict "refs";
			    $fh->{write_logfile} = "WRITELOG_$fhname";
			};
			no strict "refs";
			(syswrite ($fh->{write_logfile}, $buf, $r))
			  || (print STDERR
			      ("Cam::Scom log: Failed write to ",
			       $fh->{write_logfile},
			       ": $!"));
			use strict "refs";
		    };

		    if ($r != length ($buf))
		    {
			##  Push back amount that was not fully written
			unshift (@$list, substr ($buf, $r));
		    }
		    elsif (! $fh->{write_buf_size})
		    {
			##  Wrote all remaining data, turn off select bit
			$fd = $fh->{fd};
			vec ($$writes_requested, $fd, 1) = 0;
			if (! vec ($$reads_requested, $fd, 1))
			{
			    vec ($$errors_requested, $fd, 1) = 0;
			};
		    };

		    ##  Check for close on flush
		    if ((! $fh->{write_buf_size}) && ($fh->{close_on_flush}))
		    {
			if ($fh->{is_object})
			{
			    $fh->{handle}->close();
			}
			elsif ($fh->{is_fd_only})
			{
			    if (defined ($fh->{fd}))
			    {
				POSIX::close ($fh->{fd});
			    };
			}
			else
			{
			    no strict "refs";
			    close ($fh->{handle});
			    use strict "refs";
			};
			if (vec ($spec_mask, $fh->{fd}, 1))
			{
			    $timeout = 0;
			};
			$this->_stop_pipe ($fh, 0);
		    };
		};
	    };
	};
    };

    ##  DEBUG
##    print ::STDOUT ("_flush exited\n");
};

##  &_is_fd_used ($fd)
##    Returns true if $fd is an in-use file descriptor
sub _is_fd_used
{
    my ($fd) = shift (@_);
    my ($tmpfd);

    if (defined ($tmpfd = POSIX::dup ($fd)) &&
	($tmpfd >= 0))
    {
	POSIX::close ($tmpfd);
	return (1);
    }
    else
    {
	return (0);
    };
};

##  ->_find_bad_fd()
##    Finds a bad filehandle, one whose fd cannot be selected,
##    and returns its Cam::Scom::Fh object.  Returns -1 if none found.
sub _find_bad_fh
{
    my Cam::Scom $this = shift (@_);
    my Cam::Scom::Fh $fhobj;
    my ($fd, $fhindex);
    my ($fd_to_fh) = $this->{fd_to_fh};
    my ($fhs) = $this->{fhs};
    my ($fhname_to_index) = $this->{fhname_to_index};
    foreach $fhindex (values (%$fhname_to_index))
    {
	$fhobj = $fhs->[$fhindex];
	$fd = $fhobj->{fd};
	if (defined ($fd) && (! _is_fd_used ($fd)))
	{
	    return ($fhobj);
	};
    };
    return (undef);
};

sub _masks_to_array
{
    my Cam::Scom $this = shift (@_);
    my ($mask1) = shift (@_);
    my ($mask2) = shift (@_);
    my ($combined_mask) = $mask1 | $mask2;
    my ($fileno_limit) = $this->{fileno_limit};

##    return (map (vec ($combined_mask, $_, 1) ? $_ : (),
##		 0..($fileno_limit-1)));
    return (Cam::Scom::_mask_to_array_in_c ($combined_mask, $fileno_limit));
};

##  &_parse_host ($host)
##    Returns ($ok, $msg, $ip_addr) where $ip_addr is the
##    4-byte IP address for $host, which may be a host name
##    or a dot-notation IP address.  If the null string, returns
##    the IP loopback address of this host.
sub _parse_host
{
    my ($host) = shift (@_);
    my ($ip_addr);

    if (! length($host))
    {
	$ip_addr = INADDR_LOOPBACK;
    }
    elsif ($host =~ /^(\d+)\.(\d+)\.(\d+)\.(\d+)$/)
    {
	$ip_addr = pack ('C4', $1, $2, $3, $4);
    }
    elsif (! defined ($ip_addr = (gethostbyname ($host))[4]))
    {
	return (0, "unrecognized host \"$host\": $!");
    };
    return (1, "", $ip_addr);
};

##  &_parse_pipe ($pipe_spec)
##    Returns ($ok, $msg, $fh) where $fh is a Cam::Com2::Fh
##    with the following fields filled in:
## "fh",                ## filehandle name used in Cam::Com2
## "handle",            ## the perl thing that can be manipulated,
##                      ##    either the string name, or the FileHandle.
## "is_object",         ## true if "handle" is an object handle, false o.w.
## "is_fd_only",        ## true if "handle" is not real, this is only an fd
## "direction",         ## "<", ">", "+", or "*"
## "child_fd",          ## fd number to use in exec'd child if defined
## "use",               ## use flag, one of "", "?", "!"
## "close_on_unreg",    ## value 1 ==> close-on-unregister, 0 ==> o.w.
## "close_on_exec",     ## close_on_exec flag, one of "" or "."
## "read_separator",    ## input separator

sub _parse_pipe
{
    my ($pipe) = shift (@_);
    my ($type, $rest, $separator, $close_on_unreg, $is_object);

    ##  Make a new candidate Cam::Scom::Fh object
    my Cam::Scom::Fh $fh = new Cam::Scom::Fh;

    ##  First parse out the filehandle object, put the rest in $rest.
    if (($type = ref ($pipe)) eq "ARRAY")
    {
	$fh->{handle} = $pipe->[0];
	$rest = $pipe->[1];
    }
    elsif (! $type)
    {
	if ($pipe =~ /^([^<>\+\*]+)(.*)$/s)
	{
	    $fh->{handle} = $1;
	    $rest = $2;
	}
	else
	{
	    return (0, "Bad pipe spec: \"$pipe\"");
	};
    }
    else
    {
	return (0, "Bad pipe spec type: $type");
    };
    $fh->{fh} = _fh_to_name ($fh->{handle});
    if (ref ($fh->{handle}) eq "FileHandle")
    {
	$fh->{is_object} = 1;
	$fh->{is_fd_only} = 0;
    }
    elsif ($fh->{handle} =~ /^\d+$/)
    {
	$fh->{is_object} = 0;
	$fh->{is_fd_only} = 1;
    }
    else
    {
	$fh->{is_object} = 0;
	$fh->{is_fd_only} = 0;
    };

    if ($rest =~ /^([<>\+\*])(\d*)([\?\!]?)(\|?)(\.?)(.*)$/s)
    {
	$fh->{direction} = $1;
	$fh->{child_fd} = $2;
	$fh->{use} = $3;
	$close_on_unreg = $4;
	$fh->{close_on_exec} = $5;
	$separator = $6;
	if (length ($separator))
	{
	    $fh->{read_separator} = substr ($separator, 1);
	}
	else
	{
	    $fh->{read_separator} = "\n";
	};
	$fh->{separator_len} = length ($fh->{read_separator});
	$fh->{close_on_unreg} = ($close_on_unreg ne "|");
    }
    else
    {
	return (0, "Bad pipe spec: [FileHandle]$rest");
    };

    return (1, "", $fh);
};

##  &_parse_port ($port, $proto)
##    Returns ($ok, $msg, $portnum), where $portnum is
##    a numerical value for IP $port, which may be
##    passed as either a number or a service name.
##    $proto is the protocol name.
sub _parse_port
{
    my ($port) = shift (@_);
    my ($proto) = shift (@_);
    my ($portnum);
    if ($port !~ /^\d+$/)
    {
	if (! defined ($portnum = getservbyname ($port, $proto)))
	{
	    return (0, "service \"$port\" for protocol \"$proto\" not found by getservbyname: $!");
	};
	$port = $portnum;
    };
    return (1, "", $port);
};

##  &_set_blocking ($fd, $is_blocking)
##    Sets file descriptor $fd to blocking status of $is_blocking
##    Returns ($ok, $msg).
sub _set_blocking
{
    my ($fd) = shift (@_);
    my ($is_blocking) = shift (@_);
    my ($num);

    no strict "refs";
    $num = Cam::Scom::_fcntl_3arg_in_c ($fd, F_GETFL, 0);
    ($num == -1) && return (0, "fcntl F_GETFL on \"$fd\" failed: $!");
    use strict "refs";
    if ($is_blocking)
    {
	$num &= (~O_NDELAY);
    }
    else
    {
	$num |= O_NDELAY;
    };
    no strict "refs";
    (Cam::Scom::_fcntl_3arg_in_c ($fd, F_SETFL, $num + 0) == -1)
      && return (0, "fcntl F_SETFL on \"$fd\" failed: $!");
    use strict "refs";
    return (1, "");
};

##  &_set_close_on_exec ($fd, $do_close)
##    Sets file descriptor $fd to close-on-exec status $do_close.
##    Returns ($ok, $msg).
sub _set_close_on_exec
{
    my ($fd) = shift (@_);
    my ($do_close) = shift (@_);
    my ($num);

    no strict "refs";
    (Cam::Scom::_fcntl_3arg_in_c ($fd, F_SETFD, $do_close + 0) == -1)
      && return (0, "fcntl F_SETFD on \"$fd\" failed: $!");
    use strict "refs";
    return (1, "");
};

sub _show_mask
{
    my ($mask) = shift (@_);
    my (@result) = ();
    my ($bits) = length ($mask) * 8;
    my ($i);
    for ($i = 0; $i < $bits; ++$i)
    {
	push (@result, vec ($mask, $i, 1) ? "1" : "0");
    };
    return (join ("", @result));
};

##  _stop_pipe ($fh, $err_number)
##    Records a termination of Cam::Scom::Fh $fh with error number $err_number.
sub _stop_pipe
{
    my Cam::Scom $this = shift (@_);
    my Cam::Scom::Fh $fh = shift (@_);
    my ($err_number) = shift (@_);
    my ($fd);

    defined ($fh->{term}) || ($fh->{term} = $err_number);
    $this->{fh_has_event}{$fh->{fh}} = 1;
    $fh->{read_open} = 0;
    $fd = $fh->{fd};
    vec ($this->{read_mask}, $fd, 1) = 0;
    vec ($this->{write_mask}, $fd, 1) = 0;
    vec ($this->{error_mask}, $fd, 1) = 0;
    $fh->{write_open} = 0;
    $fh->{fd} = undef;
    vec ($this->{all_mask}, $fd, 1) = 0;
    $this->{fd_to_fh}[$fd] = undef;

    ##  DEBUG
##    print STDOUT ("Removing fd=" . $fd . " (" . $fh->{fh} . ") via _flush\n");
};

=head1 AUTHOR

Doug Campbell, C<< <doug.campbell at pobox.com> >>

=head1 SUPPORT

You can find documentation for this module with the perldoc command.

    perldoc Cam::Scom

=head1 COPYRIGHT & LICENSE

Copyright 2006,2007,2009 Doug Campbell

This file is part of Cam-Scom.

Cam-Scom is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Cam-Scom is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cam-Scom; if not, see <http://www.gnu.org/licenses/>.

=cut

bootstrap Cam::Scom;

1; # End of Cam::Scom

__END__

