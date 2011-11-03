package Satc;      ##  -*-CPerl-*-

use Exporter 'import';
require DynaLoader;
@ISA = qw(DynaLoader);
use Data::Dumper;

use warnings;
use strict;

##  Members of a Satc object:
##    {"msg_to_type"}          -- maps first msg word to its SATC msgtype
##    {"type_to_msg"}          -- maps SATC msgtype to its first msg word
##    {"type_to_format"}       -- maps SATC msgtype to its format
##    {"field_to_datatype"}    -- maps SATC fields to datatypes (DATATYPE_*)
##    {"field_to_decode_conv"} -- maps SATC fields to decode converter fns
##    {"field_to_encode_conv"} -- maps SATC fields to encode converter fns
##    {"played_reason_description"}    -- describes played reason coces (array)
##    {"recorded_reason_description"}  -- describes recorded reason coces (")
##    {"gotdigit_reason_description"}  -- describes gotdigit reason coces (")
##    {"recognized_reason_description"}  -- describes recognized reason coces (")

=head1 NAME

Satc - Simple ASCII Telephony Control

=head1 VERSION

Version 0.12

=cut

our $VERSION = '0.12';

=head1 SYNOPSIS

SATC is a telephony protocol used by phoneglue and voiceglue.
It defines messages that pass between a provider (server) of telephony
services and a user (client) of telephony services.
This package provides constants defining the SATC message types
(e.g. Satc::START, Satc::STOP, ...) and methods for
encoding and decoding SATC messages.

    use Satc;

    my ($ok, $msg, $bytes);
    my $proto = Satc->new();
    my $satc_message = {Satc::MSGTYPE => Satc::START};
    ($ok, $msg, $bytes) = $proto->encode ($satc_message);
    $ok || die ($msg);
    print ::FH ($bytes);
    $bytes = <::FH>
    ($ok, $msg, $satc_message) = $proto->decode ($bytes);
    $ok || die ($msg);
    ($satc_message->{Satc::STATUS} == 0) || die ($satc_message->{Satc::MSG});

    $hashed = Satc::hashify ($to_hash, $preserve_prefix_count, 
                             $suffix_blocks_count);

=head2 Message Representation

A SATC message is represented by a simple hashref mapping field names
to their values.  The field types, field names, and the field name
constant definitions are as follows:

    type     field        fieldname-constant
    ----     -----        ------------------
  Common to messages from clients and servers:
    int     msgtype       MSGTYPE      //  one of the msgtype constants:
      //  START, STARTED, STOP, STOPPED, REG, REGED, UNREG, UNREGED,
      //  ALLOWIN, ALLOWEDIN, STOPIN, STOPPEDIN, MAKECALL, MADECALL, INCOMING,
      //  ANSWER, ANSWERED, HANGUP, HUNGUP, PLAY, PLAYED, RECORD, RECORDED,
      //  CLEARDIG, DIGCLEARED, GETDIG, GOTDIG, PLAYDIG, PLAYEDDIG, STOPMEDIA,
      //  SUPPORTS, SUPPORTED, MAKECONF, MADECONF, JOINCONF, JOINEDCONF,
      //  EXITCONF, EXITEDCONF, BRIDGE, BRIDGED, UNBRIDGE, UNBRIDGED,
      //  TRANSFER, TRANSFERED, ASRSTART, ASRSTARTED, ASRSTOP, ASRSTOPPED,
      //  LOADGRAMMAR, LOADEDGRAMMAR, UNLOADGRAMMAR, UNLOADEDGRAMMAR,
      //  STARTGRAMMAR, STARTEDGRAMMAR, STOPGRAMMAR, STOPPEDGRAMMAR,
      //  RECOGNIZE, RECOGNIZED, RETURNVAR, RETURNEDVAR, RELEASE, RELEASED
    int      callid       CALLID       //  for almost all
    String   digits       DIGITS       //  for playdig, gotdig, recorded
    int      makecall_id  MAKECALL_ID  //  for makecall, madecall
    int      confid       CONFID       //  for makeconf and joinconf
    String   grammarname  GRAMMARNAME  //  for loadgrammar, loadedgrammar,
                                               unloadgrammar, unloadedgrammar,
                                               startgrammar, startedgrammar,
                                               stopgrammar, stoppedgrammar,
  For messages from clients only:
    String   toself       TOSELF       //  for start
    String   url          URL          //  for reg, unreg, makecall (to),
                                               asrstart
    String   from         FROM         //  for makecall (from), transfer
    String   file         FILE         //  for record, loadgrammar
    String[] files        FILES        //  for play
    String   stopkeys     STOPKEYS     //  for play, record, getdig
    int      load         LOAD         //  for reg load
    int      timeout      TIMEOUT      //  for makecall getdig record transfer
    int      max_time     MAX_TIME     //  for record
    int      max_dig      MAX_DIG      //  for getdig
    int      start_tone   START_TONE   //  for record
    int      other_callid OTHER_CALLID //  for bridge
    String   mix          MIX          //  for record
    int      flag         FLAG         //  for recognize
    String   var	  VAR	       //  for returnvar
    String   value	  VALUE	       //  for returnvar
  For messages from servers only:
    int      status       STATUS
    String   msg          MSG
    String   ani          ANI          // for incoming
    String   dnis         DNIS         // for incoming
    String   arg          ARG          // for incoming
    int      reason       REASON       // for played, recorded, gotdig,
                                          recognized
    int      duration     DURATION     // for recorded

Note that if the above pre-defined constants are imported and
then used as field names within a SATC message without a leading
package specifier, you must put () after the field
name to prevent them from being interpreted as string literals:

               use Satc qw(STOPKEYS);
  right:       $satc_message->{Satc::FILE()} = "foo.wav";
  right:       $satc_message->{Satc::FILE} = "foo.wav";
  right:       $satc_message->{STOPKEYS()} = "#";
  wrong:       $satc_message->{STOPKEYS} = "#";

Additional constants are defined for the "reason" field values:

    PLAYEDFILE_REASON_UNDETERMINED
    PLAYEDFILE_REASON_END_OF_DATA
    PLAYEDFILE_REASON_DTMF_PRESSED
    PLAYEDFILE_REASON_STOP_REQUESTED
    RECORDED_REASON_UNDETERMINED
    RECORDED_REASON_SILENCE
    RECORDED_REASON_DTMF_PRESSED
    RECORDED_REASON_STOP_REQUESTED
    RECORDED_REASON_MAX_TIME
    RECORDED_REASON_NO_AUDIO_TIMEOUT
    RECORDED_REASON_HANGUP
    GOTDIGIT_REASON_UNDETERMINED
    GOTDIGIT_REASON_TERM_PRESSED
    GOTDIGIT_REASON_MAX_DIGITS
    GOTDIGIT_REASON_TIMEOUT
    GOTDIGIT_REASON_STOP_REQUESTED
    RECOGNIZED_REASON_UNDETERMINED
    RECOGNIZED_REASON_NOMATCH
    RECOGNIZED_REASON_DTMF_PRESSED
    RECOGNIZED_REASON_STOP_REQUESTED
    RECOGNIZED_REASON_NO_AUDIO_TIMEOUT
    RECOGNIZED_REASON_HANGUP


=head2 The SATC wire protocol

  Message Format
  --------------
  SATC servers listen for TCP connections on port 44647.
  SATC clients connect to SATC servers using TCP.
  Every message is terminated by a \n (ASCII 10).
  Every message contains one or more fields.
  Fields begin at non-whitespace characters.  Fields end at
  whitespace characters unless the whitespace appears between
  non-escaped matching quote characters " (ASCII 34) or ' (ASCII 39)
  in which case the field characters include the whitespace.
  An unescaped quote character of one type within another's is not
  treated specially, but is treated as if it were escaped.
  Escaping rules are:
    \\ = "\" (ASCII 92)
    \n = "\n" (ASCII 10)
    \" = """ (ASCII 34)
    \' = "'" (ASCII 39)
    \xDD = ASCII character corresponding to hex digits DD
  A backslash (ASCII 92) followed by any other character
  sequence is an error.  Escaping rules apply to any character sequence
  or byte field, even those not enclosed by quote characters.

  The first field in a SATC message is always the message type.
  This type defines the meanings of subsequent data elements (if any).
  Recipients of SATC messages are required to ignore any message types they
  do not process, and to ignore any additional fields in messages that
  they do not use.

  Data types are implicit in SATC messages, and apply only
  to interpretation, not to representation.
  String types are interpreted as sequences of bytes.
  Integer types are interpreted as ASCII decimal numbers.

  Client -> Server Message Types
  ------------------------------
      NOTE:  All messages from client to server will
             result in a response message (described below).
             Response messages will be sent upon completion of
             the request.

 * = not yet implemented

    Type            Additional Data                       Description
    ----            ---------------                       -----------
    start           toself                                Init connection
    stop	    <none>			          End connection
    reg             string url, int load                  Register DID
    unreg           string url                            Unregister DID
    allowin         <none>                                Allow incoming
    stopin          <none>                                Stop incoming
    answer          int callid                            Answer a call
    makecall        string url, string from, int timeout, Make a new call
  			  int makecall_id
    hangup          int callid                            Hangup a call
    play            int callid, string files,             Play a file
                            string stopkeys
    record          int callid, string file,              Record to file
                            string start_tone,
                            int timeout, int max_time,
                            string stopkeys, string mix
    cleardig        int callid                            Clear digit buf
    getdig          int callid, int timeout,              Get digits
                            string stopkeys, int max_dig      collected
    playdig         int callid, string digits             Play digits
    stopmedia       int callid                            stop playing
    supports        <none>				  show supported msgs
 *  makeconf        int callid, int confid                Make a conference
 *  joinconf        int callid, int confid,               Join a conference
 *  exitconf        int callid                            Exit a conference
    bridge          int callid, int req_callid	          Bridge calls
    unbridge        int callid			          Unbridge calls
    transfer        int callid, string url, int flag,     Transfer
			string from, int timeout
    asrstart        int callid, string url                Init ASR for call
    asrstop         int callid                            Stop ASR on call
    loadgrammar     int callid, string grammarname,       Load ASR grammar
                             string file
    unloadgrammar   int callid, string grammarname        Unload ASR grammar
    startgrammar    int callid, string grammarname,       Activate ASR grammar
    stopgrammar     int callid, string grammarname        DeActivate ASR grammar
    recognize       int callid, string files,             Do speech recognition
                             int timeout, int flag
    returnvar	    int callid, string var, string value  Return values to ast
    release         int callid                            exit call, no hangup

  Server -> Client Message Types
  ------------------------------
      NOTE:  All messages except incoming, hungup, exitedconf, and unbridged
             are responses to client request messages.
  	       The incoming, hungup, exitedconf, and unbridged messages
             are asynchronous event messages or responses to requests.

      NOTE:  All (int status, string msg) pairs indicate a success/failure
             result code in status, along with an (empty if success)
             error message in msg.  status == 0 is success, != 0 is failure.

 * = not yet implemented

    Type        Additional Data
    ----        ---------------
    started     int status, string msg
    stopped     int status, string msg
    reged       int status, string msg
    unreged     int status, string msg
    allowedin   int status, string msg
    stoppedin   int status, string msg
    incoming    int callid, string ani, string dnis, string arg
    hungup      int callid
    answered    int callid, int status, string msg
    madecall    int callid, int makecall_id, int status, string msg
    played      int callid, int status, string msg, int reason
    recorded    int callid, int status, string msg, int reason, int duration, string digits
    digcleared  int callid, int status, string msg
    gotdig      int callid, int status, string msg, int reason, string digits
    playeddig   int callid,int status, string msg
 *  supported   int msg1, string msg1_format, int msg2, ...
 *  madeconf    int callid, int status, string msg, int confid
 *  joinedconf  int callid, int status, string msg
 *  exitedconf  int callid
    bridged     int callid, int status, string msg
    unbridged   int callid
    transfered  int callid, int status, string msg
    asrstarted  int callid, int status, string msg
    asrstopped  int callid, int status, string msg
    loadedgrammar    int callid, int status, string msg, string grammarname
    unloadedgrammar  int callid, int status, string msg, string grammarname
    startedgrammar   int callid, int status, string msg, string grammarname
    stoppedgrammar   int callid, int status, string msg, string grammarname
    recognized       int callid, int status, string msg, int reason
    returnedvar      int callid
    released         int callid


  Message Synchronization
  -----------------------
  All client request messages will ultimately be reponded to with their
  corresponding response messages with the following exception:

  * Hungup Exception:   After a client has sent any of the request
  			  messages answer, play,
  			  record, cleardig, getdig, playdig,
  			  makeconf, joinconf, exitconf, bridge, or
  			  unbridge, it could receive a
  			  hungup message from the server for
  			  that callid instead of the corresponding
  			  response.  The hungup message is the
  			  last message that will be sent from the server
  			  regarding that callid.  It must be accepted by a
  			  client as a terminating response for the above
  			  listed requests.

  Messages are further categorized below into four classes:

    config:       start stop reg unreg allowin stopin supports
    callcontrol:  answer hangup makecall returnvar release
    media:        play record cleardig getdig playdig stopmedia
    conference:   makeconf joinconf exitconf bridge unbridge

  It is illegal for a client to have more than one outstanding request
  in a category for a callid (or overall for the config class).
  The one exception is stopmedia which may be
  issued at any time.

  It is illegal for a client to send any media or
  conference class messages for a call until a successful
  answered or madecall message is received for that call.
  An exception is that play can occur on incoming calls
  before an answer is processed.


  Details of Messages
  -------------------
  msg:        start
  direction:  Client -> Server
  class:      config
  parameters: toself
  function:   This must be the first message sent by a client to a server.
              The client must not send any more messages until a started
              message is received in response.
	      The toself parameter specifies how outbound calls and
	      redirects are to be addressed back into the telephony provider.
	      It can be the empty string if no outbound calls or redirects
	      (used by stopmedia) are used, otherwise for asterisk specify
	      "context:extension:priority".

  msg:        started
  direction:  Server -> Client
  class:      config
  parameters: int status         -- 0 on success, non-0 on failure.
              string msg         -- If failure, an error message
  function:   Response to a start message.

  msg:        stop
  direction:  Client -> Server
  class:      config
  parameters: <none>
  function:   This must be the last message sent by a client to a server.

  msg:        stopped
  direction:  Server -> Client
  class:      config
  parameters: int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
  function:   Response to an stop message.

  msg:        reg
  direction:  Client -> Server
  class:      config
  parameters: string url         -- The inbound phone number(s) or URI(s)
                                    this client is willing to accept
                                    incoming calls for.  String can contain
                                    wildcard character "*".  If the string
                                    contains only digits, "(", ")", "-", and
                                    wildcards, it is considered a phone
                                    number versus a URI.
              int load           -- A load relative to other clients.
                                    For example, if this client specifies a
                                    load of 10, and two other clients have
                                    each specified a load of 20 for the same
                                    DID, then this client will get 20% of the
                                    incoming calls for this DID, and the
                                    other clients will get 40% each.
  function:   Establishes that this client is willing to accept incoming
              calls destined for the specified DID(s).  None will actually
              be delivered unless and until the allowin function succeeds.
              Will always be responded to with a reged message.

  msg:        reged
  direction:  Server -> Client
  class:      config
  parameters: int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
  function:   Response to a reg message.

  msg:        unreg
  direction:  Client -> Server
  class:      config
  parameters: string url         -- The inbound phone number(s) or URI(s)
                                    this client is no longer willing to
                                    accept incoming calls for.  String has
                                    the same format as in the reg message.
                                    It must be an exact match to a previous
                                    reg message url string parameter.
  function:   Establishes that this client is no longer willing to accept
              incoming calls destined for the specified DID(s).  Calls
              will stop being delivered once the unreged success
              message is received.

  msg:        unreged
  direction:  Server -> Client
  class:      config
  parameters: int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
  function:   Response to an unreg message.

  msg:        allowin
  direction:  Client -> Server
  class:      config
  parameters: <none>
  function:   Requests that the server begin sending incoming calls to
              this client for all registered DIDs.  Incoming calls will
              begin to appear after the allowedin success response
              is received.

  msg:        allowedin
  direction:  Server -> Client
  class:      config
  parameters: int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
  function:   Response to an allowin message.

  msg:        stopin
  direction:  Client -> Server
  class:      config
  parameters: <none>
  function:   Requests that the server stop sending incoming calls to
              this client for all registered DIDs.  Incoming calls will
              stop appearing after the stoppedin success response
              is received.

  msg:        stoppedin
  direction:  Server -> Client
  class:      config
  parameters: int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
  function:   Response to a stopin message.

  msg:        incoming
  direction:  Server -> Client
  class:      <event>
  parameters: int callid         -- A unique (across all servers) identifier
                                    for this call
              string ani         -- A string describing the calling entity,
                                    for normal phone numbers a digit string.
              string dnis        -- A string describing the called entity,
                                    for normal phone numbers a digit string.
              string arg         -- The agi_network_script value passed from
                                    asterisk when the AGI call was made.
  function:   Notifies a client that it has been chosen to handle the
              callflow for this incoming call.  The call is in an unanswered
              state.

  msg:        answer
  direction:  Client -> Server
  class:      callcontrol
  parameters: int callid         -- The callid of the call to answer.
  function:   Requests that the server answer the call corresponding
              to the callid, which must match a callid provided in a
              previous incoming message.

  msg:        answered
  direction:  Server -> Client
  class:      callcontrol
  parameters: int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
  function:   Response to an answer message.

  msg:        makecall
  direction:  Client -> Server
  class:      callcontrol
  parameters: string url         -- The number or URI to call.  Must be in
                                    Asterisk form, e.g. "sip/<URL>"
  	      string from	   -- The number or URI calling from.
              int timeout        -- The number of ms to wait for answer.
  	      int makecall_id    -- A unique number generated by the client
  				      to identify this makecall request.
  function:   Requests that the server start a new call to the specified
              destination.

  msg:        madecall
  direction:  Server -> Client
  class:      callcontrol
  parameters: int callid         -- A unique (across all servers) identifier
                                    for this call
  	        int makecall_id    -- Matches the makecall_id passed in the
  				      makecall message from the client
              int status         -- The result of the makecall, one of:
                                      0 = connection succeeded
                                      1 = system failure
                                      2 = line is busy
                                      3 = no answer (timeout)
                                      4 = call rejected
              int msg            -- If system failure, an error message
  function:   Response to a makecall message.  If status == 0 (success), this
              indicated that callid identifies a new call that is connected
              to the destination requested in the makecall message.  The
              client is now responsible for callflow for this call.  If
              status == 1 (system failure), msg will contain an error msg.
              If status != 0, callid will be meaningless.

  msg:        hangup
  direction:  Client -> Server
  class:      callcontrol
  parameters: int callid         -- The callid of the call to hangup.
  function:   Requests that the server hangup the call with this callid.
              If the call is not currently connected, no response is given.
              Otherwise, the server must reply with a hungup message.

  msg:        hungup
  direction:  Server -> Client
  class:      callcontrol
  parameters: int callid         -- The callid of the hungup call.
  function:   Notifies the client that the call with this callid has
              been disconnected.  No further activity can occur for
              this callid unless and until it is reused for a new call
              (via either incoming or makecall).

  msg:        release
  direction:  Client -> Server
  class:      callcontrol
  parameters: int callid         -- The callid of the call to release
  function:   Requests that the server release the call with this callid.
              If the call is not currently connected, no response is given.
              Otherwise, the server must reply with a released message.
	      This provides a way to return control to an enclosing
              execution environment without hanging up the call.

  msg:        released
  direction:  Server -> Client
  class:      callcontrol
  parameters: int callid         -- The callid of the released call.
  function:   Notifies the client that the call with this callid has
              been released.  No further activity can occur for
              this callid unless and until it is reused for a new call
              (via either incoming or makecall).

  msg:        play
  direction:  Client -> Server
  class:      media
  parameters: int callid         -- The call on which to play the media.
              string files       -- The list of files to play.  If more than
                                    one file is specified, the names must
                                    be separated by the "|" character.
                                    Each file may have appended the string
                                    "^start=XXX" where XXX is an offset in
                                    ms into the file where play should begin.
              string stopkeys    -- An ASCII string of the keys that will
  				    stop a play, record, or getdig.
                                    Valid keys are "#", "*", "0" - "9",
                                    and if VAMD is available, "m", "h", and
                                    "t".  Keys can be in any order.
  function:   Requests that the specified media files be played on the
              specified call.  Play will be terminated on the following
              conditions:
                  1: A key has been pressed matching the stopkeys and has
                     not been cleared from the key buffer via cleardig
                     or getdig.
                  2: The files have finished playing.
                  3: A stopmedia message is received.
                  4: The call is disconnected.
              If condition 1 is matched before any media starts playing,
              a played message will be sent immediately.
              Otherwise, if conditions 1-3 subsequently terminate
              the play, a played will then be sent.  If condition 4
              terminates the play, no played will be sent, only a
              hungup will be sent.

  msg:        played
  direction:  Server -> Client
  class:      media
  parameters: int callid         -- The call on which the media files have
                                    finished playing.
              int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
              int reason         -- One of the following reason codes:
                                         0 = undetermined
                                         1 = end of data
                                         2 = terminated by DTMF
                                         3 = terminated by request (stopmedia)
  function:   Sent to indicate that a play request has finished
              playing.

  msg:        record
  direction:  Client -> Server
  class:      media
  parameters: int callid         -- The call on which to record.
              string file        -- The file into which to record.
              string start_tone  -- One of "tone" or "notone" for whether
                                    to play a start tone for the record.
              int timeout        -- Max ms of silence that stop recording.
              int max_time       -- Max ms of total recording.
              string stopkeys    -- An ASCII string of the keys that will
  				    stop a play, record, or getdig.
                                    Valid keys are "#", "*", "0" - "9",
                                    and if VAMD is available, "m", "h", and
                                    "t".  Keys can be in any order.
              string mix         -- One of "inbound" or "mix".  "mix" records
                                    a mix of both inbound and outbound audio.
                                    "inbound" records only inbound.
  function:   Requests that the incoming call audio be recorded into the
              specified media file.  Record will be terminated by any of:
                  1: A key has been pressed matching the stopkeys and has
                     not been cleared from the key buffer via cleardig
                     or getdig
                  2: Record stops based on timeout or max_time.
                  3: The call is disconnected.
              If condition 1 is matched before any media starts playing,
              a recorded message will be sent immediately.  Otherwise,
              when conditions 1-2 subsequently terminate the record,
              a recorded will then be sent.  If condition 3 terminates
              the recording, a recorded will first be sent, then a hungup.

  msg:        recorded
  direction:  Server -> Client
  class:      media
  parameters: int callid         -- The call on which the media file has
                                    finished recording.
              int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
              int reason         -- One of the following reason codes:
                                         0 = undetermined
                                         1 = terminated by silence
                                         2 = terminated by DTMF
                                         3 = terminated by request
                                         4 = max_time of recording detected
                                         5 = no audio start (noinput)
                                         6 = terminated by hangup
              int duration       -- How long the recording is in ms
              string digits      -- The digit (if any) pressed to terminate
  function:   Sent to indicate that a record request has finished
              recording.

  msg:        cleardig
  direction:  Client -> Server
  class:      media
  parameters: int callid         -- The callid of call to clear digits on.
  function:   Requests that the server forget any sensed digits that
              have been pressed by the user up to this point.  Any such
              digits cleared will not terminate subsequent plays or records,
              nor be retrievable with getdig.  The server will respond with
              a digcleared message.

  msg:        digcleared
  direction:  Server -> Client
  class:      media
  parameters: int callid         -- The callid of the call.
              int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
  function:   Response to a cleardig request.

  msg:        getdig
  direction:  Client -> Server
  class:      media
  parameters: int callid         -- The call on which to get digits.
              int timeout        -- Max ms to wait for digits, can be
                                    0 to indicate no wait.
              string stopkeys    -- An ASCII string of the keys that will
  				    stop a play, record, or getdig.
                                    Valid keys are "#", "*", "0" - "9",
                                    and if VAMD is available, "m", "h", and
                                    "t".  Keys can be in any order.
              int max_dig        -- The maximum number of digits to return.
  function:   Requests that the server return the digits that have been
              pressed on the call since the last cleardig minus those
              that have already been retrieved with getdig.
              Digits will be sent back to the client (thus terminating this
              getdig request) on the following conditions:
                  1: A key has been pressed matching the stopkeys and has
                     not been cleared from the key buffer via cleardig
                     or getdig
                  2: max_dig digits have been obtained with no stopkeys found
                  3: Timeout ms have expired without stopkeys pressed
                     or max_dig being attained.
                  4: User requested termination.
                  5: The call is disconnected.
              If conditions 1-3 are matched immediately by the existing
              digit buffer contents and/or a timeout of 0, a
              gotdig message will be sent immediately.  Otherwise,
              when conditions 1-4 subsequently terminate the collection,
              a gotdig will then be sent.  If condition 5 terminates
              the digit collection, no gotdig will be sent, only a hungup.

  msg:        gotdig
  direction:  Server -> Client
  class:      media
  parameters: int callid         -- The call on which the digit collection
                                    has finished.
              int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
              int reason         -- One of the following reason codes:
                                     0 = undetermined
                                     1 = terminated by stopdigits match.
                                     2 = terminated by max_digits received.
                                     3 = terminated by timeout ms done.
                                     4 = terminated by request.
              string digits      -- The digits retrieved, in ASCII format.
  function:   Sent to indicate that a getdig request has finished.

  msg:        playdig
  direction:  Client -> Server
  class:      media
  parameters: int callid         -- The call on which to play the digits.
              string digits      -- The ASCII-formatted digits to play.
  function:   Requests that the specified digits be played on the
              specified call.  Play will be terminated on the following
              conditions:
                  1: The digits have finished playing.
                  2: A termination request is received.
                  3: The call is disconnected.
              When conditions 1-2 subsequently terminate
              the play, a playeddig will then be sent.  If condition 3
              terminates the play, no playeddig will be sent, only a hungup.

  msg:        playeddig
  direction:  Server -> Client
  class:      media
  parameters: int callid         -- The call on which the digits have
                                    finished playing.
              int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
  function:   Sent to indicate that a playdig request has finished playing.

  msg:        stopmedia
  direction:  Client -> Server
  class:      media
  parameters: int callid         -- The callid of the call to stop play.
  function:   Requests that the server stop the current media activity.
              The server will terminate the media activity, and send
              a response appropriate for the media activity that was
              ongoing.  If no media activity is ongoing, has no effect.

  msg:        supports
  direction:  Client -> Server
  class:      config
  parameters: <none>
  function:   Requests that the server send a list of all supported
  	        messages.

  msg:        supported
  direction:  Server -> Client
  class:      config
  parameters: string msg1
  	        string msg2
  	         . . .
  	        string msgN
  function:   Sent in response to a supports message, enumerates all messages
              from the client to the server supported by this server.

  msg:        makeconf
  direction:  Client -> Server
  class:      conference
  parameters: int callid         -- The callid of the call requesting
                                    the creation of the conference.
              int confid         -- The requested id of the conference.
  function:   Requests that a new conference be created with id confid.
              The server must respond with a madeconf message.

  msg:        madeconf
  direction:  Server -> Client
  class:      conference
  parameters: int callid         -- The call that previously called makeconf.
              int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
              int confid         -- The conference identifier
  function:   Sent in response to a makeconf message.

  msg:        joinconf
  direction:  Client -> Server
  class:      conference
  parameters: int callid       -- callid of the call joining the conference.
              string confid    -- The conference identifier
  function:   Requests that the server join call callid to conference
              id confid.  The server must respond with a joinedconf message.

  msg:        joinedconf
  direction:  Server -> Client
  class:      conference
  parameters: int callid         -- The call that previously called joinconf.
              int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
  function:   Sent in response to a joinconf message to establish
              the success or failure of joining the conference.

  msg:        exitconf
  direction:  Client -> Server
  class:      conference
  parameters: int callid       -- callid of the call exiting the conference.
  function:   Requests that the server stop the specified call from
              participating in its current conference.  It can be used
              to terminate either the host or the joiner of a conference.
              If the call is not currently in a conference, no response
              is given.
              Otherwise, the server must respond with a exitedconf

  msg:        exitedconf
  direction:  Server -> Client
  class:      conference
  parameters: int callid       -- callid of the call whose conference stopped
  function:   Sent when a call stops participating in a conference.
              Sent in response to an exitconf message, or asynchronously.

  msg:	bridge
  direction:  Client -> Server
  class:	conference
  parameters: int callid       -- callid of the call that wants to bridge
  	        int req_callid   -- callid of the call to bridge to
  function:   Requests that callid bridge to req_callid.  Both callid
              and req_callid must make this call specifying the other
              in order for the bridge to succeed and audio to pass.
              If the other side does not executes a corresponding bridge,
              this channel will be stuck with no audio until an unbridge
              is performed.

  msg:        bridged
  direction:  Server -> Client
  class:      conference
  parameters: int callid       -- callid of call that requested the bridge
  	        int status
  	        string msg
  function:   Response to a bridge message.

  msg:	unbridge
  direction:  Client -> Server
  class:	conference
  parameters: int callid	  -- callid of the call that wants to unbridge
  function:   Requests that callid stop bridging.  Both calls in the bridge
              will be unbridged when either side executes an unbridge.
              Both will get an unbridged message.

  msg:        unbridged
  direction:  Server -> Client
  class:      conference
  parameters: int callid        -- callid of call that is now unbridged
  	    int status
  	    string msg
  function:   Response to an unbridge message, or async notification

  msg:	transfer
  direction:  Client -> Server
  class:	conference
  parameters: int callid       -- callid of the call that wants to transfer
              string url       -- The number or URI to call.  Must be in
                                  Asterisk form, e.g. "sip/<URL>"
	      int flag	       -- 0 = blind transfer (method=dial),
                                  1 = blind transfer (method=transfer)
                                  2 = bridged transfer (method=dial)
	      string from      -- callerid (ANI) to call from ("" = default)
	      int timeout      -- max ms to wait for connect (0 = forever)
  function:   Performs a transfer.  (For hosted transfers,
              perform an outbound call followed by a bridge).
              The transfered response message will not be
              sent until the outbound transfer call finishes.
              If the caller has not hungup, further operations
              on the call are then permitted.

  msg:        transfered
  direction:  Server -> Client
  class:      conference
  parameters: int callid       -- callid of call that requested the bridge
  	        int status
  	        string msg
  function:   Response to a bridge message.
              The call has finished the outbound leg,
              and is ready for further operation.

  msg:        asrstart
  direction:  Client -> Server
  class:      media
  parameters: int callid       -- call id of call to start ASR engine on
              string url       -- identifier of particular ASR engine
  function:   Initializes ASR engine on call.  No ASR message may be
              sent on this call  until a successful asrstarted message
              is received in response.

  msg:        asrstarted
  direction:  Server -> Client
  class:      media
  parameters: int callid         -- callid of call whose ASR engine started
              int status         -- 0 on success, non-0 on failure.
              string msg         -- If failure, an error message
  function:   Response to an asrstart message.

  msg:        asrstop
  direction:  Client -> Server
  class:      media
  parameters: int callid       -- call id of call to stop ASR engine on
  function:   Stops the ASR capability.

  msg:        asrstopped
  direction:  Server -> Client
  class:      media
  parameters: int callid         -- callid of call whose ASR engine stopped
              int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
  function:   Response to an asrstop message.

  msg:        loadgrammar
  direction:  Client -> Server
  class:      media
  parameters: int callid         -- call id of call to load grammar for
              string grammarname -- identifier of particular grammar
              string file        -- file containing grammar
  function:   Loada an SRGS speech grammar for a call for later use
              by activategrammar.

  msg:        loadedgrammar
  direction:  Server -> Client
  class:      media
  parameters: int callid         -- call id of loaded grammar
              string grammarname -- identifier of particular grammar
              int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
  function:   Response to a loadgrammar message.

  msg:        unloadgrammar
  direction:  Client -> Server
  class:      media
  parameters: int callid         -- call id of call to unload grammar for
              string grammarname -- identifier of particular grammar
  function:   Unloada an SRGS speech grammar for a call

  msg:        unloadedgrammar
  direction:  Server -> Client
  class:      media
  parameters: int callid         -- call id of unloaded grammar
              string grammarname -- identifier of particular grammar
              int status         -- 0 on success, != 0 for failure
              string msg         -- If failure, an error message
  function:   Response to an unloadgrammar message.

=cut

use constant START => 1;
use constant STOP => 2;
use constant REG => 3;
use constant UNREG => 4;
use constant ALLOWIN => 5;
use constant STOPIN => 6;
use constant ANSWER => 7;
use constant MAKECALL => 8;
use constant HANGUP => 9;
use constant PLAY => 10;
use constant RECORD => 11;
use constant CLEARDIG => 12;
use constant GETDIG => 13;
use constant PLAYDIG => 14;
use constant STOPMEDIA => 15;
use constant SUPPORTS => 16;
use constant MAKECONF => 17;
use constant JOINCONF => 18;
use constant EXITCONF => 19;
use constant BRIDGE => 20;
use constant UNBRIDGE => 21;
use constant TRANSFER => 22;
use constant ASRSTART => 23;
use constant ASRSTOP => 24;
use constant LOADGRAMMAR => 25;
use constant UNLOADGRAMMAR => 26;
use constant STARTGRAMMAR => 27;
use constant STOPGRAMMAR => 28;
use constant RECOGNIZE => 29;
use constant RETURNVAR => 30;
use constant RELEASE => 31;
use constant STARTED => 51;
use constant STOPPED => 52;
use constant REGED => 53;
use constant UNREGED => 54;
use constant ALLOWEDIN => 55;
use constant STOPPEDIN => 56;
use constant INCOMING => 57;
use constant HUNGUP => 58;
use constant ANSWERED => 59;
use constant MADECALL => 60;
use constant PLAYED => 61;
use constant RECORDED => 62;
use constant DIGCLEARED => 63;
use constant GOTDIG => 64;
use constant PLAYEDDIG => 65;
use constant SUPPORTED => 66;
use constant MADECONF => 67;
use constant JOINEDCONF => 68;
use constant EXITEDCONF => 69;
use constant BRIDGED => 70;
use constant UNBRIDGED => 71;
use constant TRANSFERED => 72;
use constant ASRSTARTED => 73;
use constant ASRSTOPPED => 74;
use constant LOADEDGRAMMAR => 75;
use constant UNLOADEDGRAMMAR => 76;
use constant STARTEDGRAMMAR => 77;
use constant STOPPEDGRAMMAR => 78;
use constant RECOGNIZED => 79;
use constant RETURNEDVAR => 80;
use constant RELEASED => 81;

use constant MSGTYPE => "msgtype";
use constant CALLID => "callid";
use constant DIGITS => "digits";
use constant MAKECALL_ID => "makecall_id";
use constant TOSELF => "toself";
use constant CONFID => "confid";
use constant URL => "url";
use constant FROM => "from";
use constant FILE => "file";
use constant FILES => "files";
use constant STOPKEYS => "stopkeys";
use constant LOAD => "load";
use constant TIMEOUT => "timeout";
use constant MAX_TIME => "max_time";
use constant MAX_DIG => "max_dig";
use constant START_TONE => "start_tone";
use constant OTHER_CALLID => "other_callid";
use constant MIX => "mix";
use constant STATUS => "status";
use constant MSG => "msg";
use constant ANI => "ani";
use constant DNIS => "dnis";
use constant ARG => "arg";
use constant REASON => "reason";
use constant DURATION => "duration";
use constant GRAMMARNAME => "grammarname";
use constant FLAG => "flag";
use constant VAR => "var";
use constant VALUE => "value";

use constant PLAYEDFILE_REASON_UNDETERMINED => 0;
use constant PLAYEDFILE_REASON_END_OF_DATA => 1;
use constant PLAYEDFILE_REASON_DTMF_PRESSED => 2;
use constant PLAYEDFILE_REASON_STOP_REQUESTED => 3;
use constant RECORDED_REASON_UNDETERMINED => 0;
use constant RECORDED_REASON_SILENCE => 1;
use constant RECORDED_REASON_DTMF_PRESSED => 2;
use constant RECORDED_REASON_STOP_REQUESTED => 3;
use constant RECORDED_REASON_MAX_TIME => 4;
use constant RECORDED_REASON_NO_AUDIO_TIMEOUT => 5;
use constant RECORDED_REASON_HANGUP => 6;
use constant GOTDIGIT_REASON_UNDETERMINED => 0;
use constant GOTDIGIT_REASON_TERM_PRESSED => 1;
use constant GOTDIGIT_REASON_MAX_DIGITS => 2;
use constant GOTDIGIT_REASON_TIMEOUT => 3;
use constant GOTDIGIT_REASON_STOP_REQUESTED => 4;
use constant RECOGNIZED_REASON_UNDETERMINED => 0;
use constant RECOGNIZED_REASON_NOMATCH => 1;
use constant RECOGNIZED_REASON_DTMF_PRESSED => 2;
use constant RECOGNIZED_REASON_STOP_REQUESTED => 3;
use constant RECOGNIZED_REASON_NO_AUDIO_TIMEOUT => 4;
use constant RECOGNIZED_REASON_HANGUP => 5;

use constant RESPONSE_OK => 0;
use constant RESPONSE_FAIL => 1;

use constant DATATYPE_UNKNOWN => 0;
use constant DATATYPE_STRING => 1;
use constant DATATYPE_INT => 2;
use constant DATATYPE_STRINGARRAYREF => 3;

##  Internal Conversion Functions

##  $string = dump_data ($item)
##    -- Returns one-line string representation of $item
sub _dump_data
{
    my ($item) = shift (@_);
    my ($obj, $result);

    $obj = new Data::Dumper ([$item], ["item"]);
    $obj->Indent (0);
    $obj->Useqq (1);
    $obj->Terse (1); 
    $result = $obj->Dump;
    chomp ($result);
    return ($result);
};

sub _decode_convert_files
{
    my ($files) = shift (@_);
    return ([split (/\|/, $files)]);
};

sub _encode_convert_files
{
    my ($files) = shift (@_);
    return (_escape_SATC_string (join ("|", @$files)));
};

=head2 _encode_convert_escape ($value)

NOTE:  This is an normally an internal routine, but may
be useful externally for quoting purposes.

Returns ($ok, $msg, $bytes) where $ok == 1 and $bytes is the
byte string of encoded SATC field $value on success,
$ok == 0 and $msg is a human-readable error message on failure.
This encoding will always surround $value with double quotes.

=cut

##  Used for strings that may need escaping
sub _encode_convert_escape
{
    my ($msg) = shift (@_);
    defined ($msg) || return ("\"\"");
    length ($msg) || return ("\"\"");
    return (_escape_SATC_string ($msg));
};

##  Used for strings that will not need escaping
sub _encode_convert_noescape
{
    my ($string) = shift (@_);
    defined ($string) || return ("\"\"");
    length ($string) || return ("\"\"");
    return ($string);
};

##  Used to describe values that may be undef
sub _describe_if_undef
{
    my ($item) = shift (@_);
    defined ($item) && return ($item);
    return "<UNDEF>";
};

=head1 FUNCTIONS

=head2 new()

Reeturns a new SATC protocol object.
SATC protocol objects have no state
(at least, they could have none),
so their only reason for existing as
objects instead of functions is to
not pollute the namespace.

=cut

sub new
{
    my $class = shift (@_);
    my $self = {};
    my ($msg, $value);

    ##  Create myself
    bless $self, $class;

    $self->{"msg_to_type"} = {"start" => START,
			      "stop" => STOP,
			      "reg" => REG,
			      "unreg" => UNREG,
			      "allowin" => ALLOWIN,
			      "stopin" => STOPIN,
			      "answer" => ANSWER,
			      "makecall" => MAKECALL,
			      "hangup" => HANGUP,
			      "play" => PLAY,
			      "record" => RECORD,
			      "cleardig" => CLEARDIG,
			      "getdig" => GETDIG,
			      "playdig" => PLAYDIG,
			      "stopmedia" => STOPMEDIA,
			      "supports" => SUPPORTS,
			      "makeconf" => MAKECONF,
			      "joinconf" => JOINCONF,
			      "exitconf" => EXITCONF,
			      "bridge" => BRIDGE,
			      "unbridge" => UNBRIDGE,
			      "transfer" => TRANSFER,
			      "asrstart" => ASRSTART,
			      "asrstop" => ASRSTOP,
			      "loadgrammar" => LOADGRAMMAR,
			      "unloadgrammar" => UNLOADGRAMMAR,
			      "startgrammar" => STARTGRAMMAR,
			      "stopgrammar" => STOPGRAMMAR,
			      "recognize" => RECOGNIZE,
			      "returnvar" => RETURNVAR,
			      "release" => RELEASE,
			      "started" => STARTED,
			      "stopped" => STOPPED,
			      "reged" => REGED,
			      "unreged" => UNREGED,
			      "allowedin" => ALLOWEDIN,
			      "stoppedin" => STOPPEDIN,
			      "incoming" => INCOMING,
			      "hungup" => HUNGUP,
			      "answered" => ANSWERED,
			      "madecall" => MADECALL,
			      "played" => PLAYED,
			      "recorded" => RECORDED,
			      "digcleared" => DIGCLEARED,
			      "gotdig" => GOTDIG,
			      "playeddig" => PLAYEDDIG,
			      "supported" => SUPPORTED,
			      "madeconf" => MADECONF,
			      "joinedconf" => JOINEDCONF,
			      "exitedconf" => EXITEDCONF,
			      "bridged" => BRIDGED,
			      "unbridged" => UNBRIDGED,
			      "transfered" => TRANSFERED,
			      "asrstarted" => ASRSTARTED,
			      "asrstopped" => ASRSTOPPED,
			      "loadedgrammar" => LOADEDGRAMMAR,
			      "unloadedgrammar" => UNLOADEDGRAMMAR,
			      "startedgrammar" => STARTEDGRAMMAR,
			      "stoppedgrammar" => STOPPEDGRAMMAR,
			      "recognized" => RECOGNIZED,
			      "returnedvar" => RETURNEDVAR,
			      "released" => RELEASED,
			     };

    ##  Compute inverse map type_to_msg
    foreach $msg (keys (%{$self->{"msg_to_type"}}))
    {
	$value = $self->{"msg_to_type"}{$msg};
	$self->{"type_to_msg"}{$value} = $msg;
    };

    $self->{"type_to_format"} =
    {
     START() => [TOSELF],
     STOP() => [],
     REG() => [URL, LOAD],
     UNREG() => [URL],
     ALLOWIN() => [],
     STOPIN() => [],
     ANSWER() => [CALLID],
     MAKECALL() => [URL, FROM, TIMEOUT, MAKECALL_ID],
     HANGUP() => [CALLID],
     PLAY() => [CALLID, FILES, STOPKEYS],
     RECORD() => [CALLID, FILE, START_TONE, TIMEOUT,
		  MAX_TIME, STOPKEYS, MIX],
     CLEARDIG() => [CALLID],
     GETDIG() => [CALLID, TIMEOUT, STOPKEYS, MAX_DIG],
     PLAYDIG() => [CALLID, DIGITS],
     STOPMEDIA() => [CALLID],
     SUPPORTS() => [],
     MAKECONF() => [CALLID, CONFID],
     JOINCONF() => [CALLID, CONFID],
     EXITCONF() => [CALLID],
     BRIDGE() => [CALLID, OTHER_CALLID],
     UNBRIDGE() => [CALLID],
     TRANSFER() => [CALLID, URL, FLAG, FROM, TIMEOUT],
     ASRSTART() => [CALLID, URL],
     ASRSTOP() => [CALLID],
     LOADGRAMMAR() => [CALLID, GRAMMARNAME, FILE],
     UNLOADGRAMMAR() => [CALLID, GRAMMARNAME],
     STARTGRAMMAR() => [CALLID, GRAMMARNAME],
     STOPGRAMMAR() => [CALLID, GRAMMARNAME],
     RECOGNIZE() => [CALLID, FILES, TIMEOUT, FLAG],
     RETURNVAR() => [CALLID, VAR, VALUE],
     RELEASE() => [CALLID],
     STARTED() => [STATUS, MSG],
     STOPPED() => [STATUS, MSG],
     REGED() => [STATUS, MSG],
     UNREGED() => [STATUS, MSG],
     ALLOWEDIN() => [STATUS, MSG],
     STOPPEDIN() => [STATUS, MSG],
     INCOMING() => [CALLID, ANI, DNIS, ARG],
     HUNGUP() => [CALLID],
     ANSWERED() => [CALLID, STATUS, MSG],
     MADECALL() => [CALLID, MAKECALL_ID, STATUS, MSG],
     PLAYED() => [CALLID, STATUS, MSG, REASON],
     RECORDED() => [CALLID, STATUS, MSG, REASON, DURATION, DIGITS],
     DIGCLEARED() => [CALLID, STATUS, MSG],
     GOTDIG() => [CALLID, STATUS, MSG, REASON, DIGITS],
     PLAYEDDIG() => [CALLID, STATUS, MSG],
     SUPPORTED() => [],
     MADECONF() => [CALLID, STATUS, MSG, CONFID],
     JOINEDCONF() => [CALLID, STATUS, MSG],
     EXITEDCONF() => [CALLID],
     BRIDGED() => [CALLID, STATUS, MSG],
     UNBRIDGED() => [CALLID],
     TRANSFERED() => [CALLID, STATUS, MSG],
     ASRSTARTED() => [CALLID, STATUS, MSG],
     ASRSTOPPED() => [CALLID, STATUS, MSG],
     LOADEDGRAMMAR() => [CALLID, GRAMMARNAME, STATUS, MSG],
     UNLOADEDGRAMMAR() => [CALLID, GRAMMARNAME, STATUS, MSG],
     STARTEDGRAMMAR() => [CALLID, GRAMMARNAME, STATUS, MSG],
     STOPPEDGRAMMAR() => [CALLID, GRAMMARNAME, STATUS, MSG],
     RECOGNIZED() => [CALLID, REASON, STATUS, MSG],
     RETURNEDVAR() => [CALLID],
     RELEASED() => [CALLID],
    };

    $self->{"field_to_datatype"} =
    {
     MSGTYPE() => DATATYPE_INT,
     CALLID() => DATATYPE_INT,
     DIGITS() => DATATYPE_STRING,
     TOSELF() => DATATYPE_STRING,
     MAKECALL_ID() => DATATYPE_INT,
     CONFID() => DATATYPE_INT,
     URL() => DATATYPE_STRING,
     FROM() => DATATYPE_STRING,
     FILE() => DATATYPE_STRING,
     FILES() => DATATYPE_STRINGARRAYREF,
     STOPKEYS() => DATATYPE_STRING,
     LOAD() => DATATYPE_INT,
     TIMEOUT() => DATATYPE_INT,
     MAX_TIME() => DATATYPE_INT,
     MAX_DIG() => DATATYPE_INT,
     START_TONE() => DATATYPE_INT,
     OTHER_CALLID() => DATATYPE_INT,
     MIX() => DATATYPE_STRING,
     STATUS() => DATATYPE_INT,
     MSG() => DATATYPE_STRING,
     ANI() => DATATYPE_STRING,
     DNIS() => DATATYPE_STRING,
     ARG() => DATATYPE_STRING,
     REASON() => DATATYPE_INT,
     DURATION() => DATATYPE_INT,
     GRAMMARNAME() => DATATYPE_STRING,
     FLAG() => DATATYPE_INT,
    };

    $self->{"field_to_decode_conv"} =
    {
     FILES() => \&_decode_convert_files,
    };

    $self->{"field_to_encode_conv"} =
    {
     MSG() => \&_encode_convert_escape,
     ANI() => \&_encode_convert_noescape,
     DNIS() => \&_encode_convert_noescape,
     TOSELF() => \&_encode_convert_escape,
     ARG() => \&_encode_convert_escape,
     DIGITS() => \&_encode_convert_noescape,
     URL() => \&_encode_convert_escape,
     FROM() => \&_encode_convert_escape,
     FILE() => \&_encode_convert_escape,
     FILES() => \&_encode_convert_files,
     STOPKEYS() => \&_encode_convert_noescape,
     MIX() => \&_encode_convert_noescape,
     GRAMMARNAME() => \&_encode_convert_escape,
    };

    $self->{"played_reason_description"} =
      [
       "undetermined",
       "end-of-data",
       "DTMF-pressed",
       "stop-requested"
      ];
    $self->{"recorded_reason_description"} =
      [
       "undetermined",
       "max-silence",
       "DTMF-pressed",
       "stop-requested",
       "max-time",
       "no-audio-timeout",
       "hangup"
      ];
    $self->{"gotdigit_reason_description"} =
      [
       "undetermined",
       "stopdigit-DTMF",
       "max-digits",
       "timeout",
       "stop-requested"
      ];
    $self->{"recognized_reason_description"} =
      [
       "undetermined",
       "no-match",
       "DTMF-pressed",
       "stop-requested",
       "no-audio-timeout",
       "hangup"
      ];

    return $self;
};

=head1 METHODS

=head2 ->protocol_number()

Returns the unique (to phoneglue) protocol number
of the SATC protocol, the number 1.

=cut

sub protocol_number
{
    return 1;
};

=head2 ->protocol_name()

Returns the unique (to phoneglue) protocol name
of the SATC protocol, "SATC".

=cut

sub protocol_name
{
    return "SATC";
};

=head2 ->magic_byte()

Returns the unique (to phoneglue) first byte of
the first message received from the SATC protocol.

=cut

sub magic_byte
{
    my ($self) = shift (@_);
    return substr ($self->{"type_to_msg"}{Satc::START}, 0, 1);
};

=head2 ->describe_msgtype ($msgtype)

Returns a human-readable description of SATC message type $msgtype

=cut

##  describe_msgtype ($msgtype)
##    --  Returns string describing the SATC message type
##        contained in $msgtype
sub describe_msgtype
{
    my ($self) = shift (@_);
    my ($msgtype) = shift (@_);
    my ($description);

    defined ($description = $self->{"type_to_msg"}{$msgtype})
      || return ("UNDEFINED");
    return $description;
};


=head2 ->describe_played_reason ($reason)

Returns a human-readable description of SATC played reason code

=cut

##  describe_played_reason ($reason)
##    --  Returns string describing the SATC played $reason
sub describe_played_reason
{
    my ($self) = shift (@_);
    my ($reason) = shift (@_);
    return
      _describe_if_undef
	($self->{"played_reason_description"}[$reason]);
};

=head2 ->describe_recorded_reason ($reason)

Returns a human-readable description of SATC recorded reason code

=cut

##  describe_recorded_reason ($reason)
##    --  Returns string describing the SATC recorded $reason
sub describe_recorded_reason
{
    my ($self) = shift (@_);
    my ($reason) = shift (@_);
    return
      _describe_if_undef
	($self->{"recorded_reason_description"}[$reason]);
};

=head2 ->describe_gotdigit_reason ($reason)

Returns a human-readable description of SATC gotdigit reason code

=cut

##  describe_gotdigit_reason ($reason)
##    --  Returns string describing the SATC gotdigit $reason
sub describe_gotdigit_reason
{
    my ($self) = shift (@_);
    my ($reason) = shift (@_);
    return
      _describe_if_undef
	($self->{"gotdigit_reason_description"}[$reason]);
};

=head2 ->describe_recognized_reason ($reason)

Returns a human-readable description of SATC recognized reason code

=cut

##  describe_recognized_reason ($reason)
##    --  Returns string describing the SATC recognized $reason
sub describe_recognized_reason
{
    my ($self) = shift (@_);
    my ($reason) = shift (@_);
    return
      _describe_if_undef
	($self->{"recognized_reason_description"}[$reason]);
};

=head2 ->describe_msg ($msg)

Returns a human-readable description of SATC message $msg

=cut

##  describe_msg ($msg)
##    --  Returns string describing the SATC message
##        contained in hash $msg
sub describe_msg
{
    my ($self) = shift (@_);
    my ($msg) = shift (@_);
    my (@result) = ();
    my ($i, $type);

    $type = $msg->{"msgtype"};
    push (@result, _describe_if_undef ($self->{"type_to_msg"}{$type}));
    if (($type == ANSWER) ||
	($type == PLAY) ||
	($type == RECORD) ||
	($type == CLEARDIG) ||
	($type == GETDIG) ||
	($type == PLAYDIG) ||
	($type == STOPMEDIA) ||
	($type == MAKECONF) ||
	($type == JOINCONF) ||
	($type == EXITCONF) ||
	($type == BRIDGE) ||
	($type == UNBRIDGE) ||
	($type == TRANSFER) ||
	($type == INCOMING) ||
	($type == HUNGUP) ||
	($type == ANSWERED) ||
	($type == MADECALL) ||
	($type == PLAYED) ||
	($type == RECORDED) ||
	($type == DIGCLEARED) ||
	($type == GOTDIG) ||
	($type == PLAYEDDIG) ||
	($type == MADECONF) ||
	($type == JOINEDCONF) ||
	($type == EXITEDCONF) ||
	($type == BRIDGED) ||
	($type == UNBRIDGED) ||
	($type == TRANSFERED) ||
        ($type == ASRSTART) ||
        ($type == ASRSTOP) ||
        ($type == ASRSTARTED) ||
        ($type == ASRSTOPPED) ||
        ($type == LOADGRAMMAR) ||
        ($type == UNLOADGRAMMAR) ||
        ($type == LOADEDGRAMMAR) ||
        ($type == UNLOADEDGRAMMAR) ||
        ($type == STARTGRAMMAR) ||
        ($type == STOPGRAMMAR) ||
        ($type == STARTEDGRAMMAR) ||
        ($type == STOPPEDGRAMMAR) ||
        ($type == RECOGNIZE) ||
        ($type == RECOGNIZED) ||
        ($type == RETURNVAR) ||
        ($type == RETURNEDVAR) ||
        ($type == RELEASE) ||
        ($type == RELEASED))
    {
	push (@result, " callid=", _describe_if_undef ($msg->{CALLID()}));
    };
    if (($type == STARTED) ||
	($type == STOPPED) ||
	($type == UNREGED) ||
	($type == ALLOWEDIN) ||
	($type == STOPPEDIN) ||
	($type == ANSWERED) ||
	($type == MADECALL) ||
	($type == PLAYED) ||
	($type == RECORDED) ||
	($type == DIGCLEARED) ||
	($type == GOTDIG) ||
	($type == PLAYEDDIG) ||
	($type == SUPPORTED) ||
	($type == MADECONF) ||
	($type == JOINEDCONF) ||
	($type == BRIDGED) ||
	($type == UNBRIDGED) ||
        ($type == TRANSFERED) ||
        ($type == ASRSTARTED) ||
        ($type == ASRSTOPPED) ||
        ($type == LOADEDGRAMMAR) ||
        ($type == UNLOADEDGRAMMAR) ||
        ($type == STARTEDGRAMMAR) ||
        ($type == STOPPEDGRAMMAR) ||
        ($type == RECOGNIZED))
    {
	push (@result, " status=", _describe_if_undef ($msg->{STATUS()}),
	      " msg=\"", $msg->{MSG()}, "\"");
    };
    if ($type == INCOMING)
    {
	push (@result, " ani=\"", _describe_if_undef ($msg->{ANI()}),
	      "\" dnis=\"", _describe_if_undef ($msg->{DNIS()}),
	      "\" arg=\"", _describe_if_undef ($msg->{ARG()}), "\"",
	     );
    }
    elsif ($type == START)
    {
	push (@result, " toself=",
	      _describe_if_undef ($msg->{TOSELF()}));
    }
    elsif ($type == MADECALL)
    {
	push (@result, " makecall_id=",
	      _describe_if_undef ($msg->{MAKECALL_ID()}));
    }
    elsif ($type == GOTDIG)
    {
	if (defined ($msg->{DIGITS()}))
	{
	    push (@result, " digits=\"",
		  _describe_if_undef ($msg->{DIGITS()}), "\"");
	};
	if (defined ($msg->{REASON()}))
	{
	    push (@result, " reason=",
		  $self->describe_gotdigit_reason ($msg->{REASON()}));
	};
    }
    elsif ($type == MADECONF)
    {
	push (@result, " confid=\"",
	      _describe_if_undef ($msg->{CONFID()}), "\"");
    }
    elsif ($type == PLAYED)
    {
	if (defined ($msg->{REASON()}))
	{
	    push (@result, " reason=",
		  $self->describe_played_reason ($msg->{REASON()}));
	};
    }
    elsif ($type == RECORDED)
    {
	if (defined ($msg->{REASON()}))
	{
	    push (@result, " reason=",
		  $self->describe_recorded_reason ($msg->{REASON()}));
	};
	if (defined ($msg->{DURATION()}))
	{
	    push (@result, " duration=", $msg->{DURATION()});
	};
	if (defined ($msg->{DIGITS()}))
	{
	    push (@result, " digits=", $msg->{DIGITS()});
	};
    }
    elsif ($type == REG)
    {
	push (@result, " url=\"",
	      _describe_if_undef ($msg->{URL()}),
	      "\", load=",
	      _describe_if_undef ($msg->{LOAD()}));
    }
    elsif (($type == UNREG) ||
	   ($type == ASRSTART))
    {
	push (@result, " url=\"",
	      _describe_if_undef ($msg->{URL()}), "\"");
    }
    elsif ($type == MAKECALL)
    {
	push (@result, " makecall_id=\"",
	      _describe_if_undef ($msg->{MAKECALL_ID()}),
	      "\" url=\"", _describe_if_undef ($msg->{URL()}),
	      "\" from=\"", _describe_if_undef ($msg->{FROM()}),
	      "\"\" timeout=", _describe_if_undef ($msg->{TIMEOUT()}));
    }
    elsif (($type == PLAY) || ($type == RECOGNIZE))
    {
	push (@result, " files=(");
	if (! defined ($msg->{FILES()}))
	{
	    die ("No files member found in " . $self->{"type_to_msg"}{$type} .
		 " message: " . _dump_data ($msg));
	};
	for ($i = 0; $i < scalar (@{$msg->{FILES()}}); ++$i)
	{
	    if ($i != 0)
	    {
		push (@result, ", ");
	    };
	    push (@result, _dump_data ($msg->{FILES()}[$i]));
	};
	push (@result, ")");
	if ($type == PLAY)
	{
	    push (@result, " stopkeys=\"",
		  _describe_if_undef ($msg->{STOPKEYS()}), "\"");
	}
	else  ##  $type == RECOGNIZE
	{
	    push (@result, " timeout=", $msg->{TIMEOUT()});
	    push (@result, " flag=", $msg->{FLAG()});
	};
    }
    elsif ($type == RECORD)
    {
	push (@result, " file=\"", _describe_if_undef ($msg->{FILE()}),
	      "\" start_tone=", _describe_if_undef ($msg->{START_TONE()}),
	      " timeout=", _describe_if_undef ($msg->{TIMEOUT()}),
	      " max_time=", _describe_if_undef ($msg->{MAX_TIME()}),
	      " stopkeys=\"", _describe_if_undef ($msg->{STOPKEYS()}),
	      "\" mix=\"", _describe_if_undef ($msg->{MIX()}), "\"");
    }
    elsif ($type == GETDIG)
    {
	push (@result, " timeout=", _describe_if_undef ($msg->{TIMEOUT()}),
	      " stopkeys=\"", _describe_if_undef ($msg->{STOPKEYS()}),
	      "\" max_dig=", _describe_if_undef ($msg->{MAX_DIG()}));
    }
    elsif ($type == PLAYDIG)
    {
	push (@result, " digits=\"",
	      _describe_if_undef ($msg->{DIGITS()}), "\"");
    }
    elsif ($type == MAKECONF)
    {
	push (@result, " confid=", _describe_if_undef ($msg->{CONFID()}));
    }
    elsif ($type == JOINCONF)
    {
	push (@result, " confid=", _describe_if_undef ($msg->{CONFID()}));
    }
    elsif ($type == BRIDGE)
    {
	push (@result, " req_callid=",
	      _describe_if_undef ($msg->{OTHER_CALLID()}));
    }
    elsif ($type == TRANSFER)
    {
	my ($flag_value, $flag_description, $from);
	$from = $msg->{FROM()};
	defined ($flag_value = $msg->{FLAG()}) || ($flag_value = 0);
	$flag_description = "0x" . sprintf ("%04x", $flag_value);
	if ($flag_value == 0)
	{
	    $flag_description .= " (blind, method=dial)";
	}
	elsif ($flag_value == 1)
	{
	    $flag_description .= " (blind, method=transfer)";
	}
	else
	{
	    $flag_description .= " (bridged, method=dial)";
	};
	push (@result, " url=\"", _describe_if_undef ($msg->{URL()}), "\"");
	push (@result, " flag=", $flag_description);
	push (@result, " from=\"", _describe_if_undef ($msg->{FROM()}), "\"");
	push (@result, " timeout=", _describe_if_undef ($msg->{TIMEOUT()}));
    }
    elsif (($type == LOADGRAMMAR) ||
	   ($type == UNLOADGRAMMAR) ||
	   ($type == LOADEDGRAMMAR) ||
	   ($type == UNLOADEDGRAMMAR) ||
	   ($type == STARTGRAMMAR) ||
	   ($type == STOPGRAMMAR) ||
	   ($type == STARTEDGRAMMAR) ||
	   ($type == STOPPEDGRAMMAR))
    {
	push (@result, " grammarname=\"", $msg->{GRAMMARNAME()}, "\"");
	if ($type == LOADGRAMMAR)
	{
	    push (@result, " file=\"", $msg->{FILE()}, "\"");
	};
    }
    elsif ($type == RECOGNIZED)
    {
	push (@result, " reason=",
	      $self->describe_recognized_reason ($msg->{REASON()}));
	##  ZZZZZZZZZZZZZZZ
    }
    elsif ($type == RETURNVAR)
    {
	push (@result, " var=",
	      $msg->{VAR()}, " value=", $msg->{VALUE()});
    };
    return (join ("", @result));
};

=head2 ->extract_message (\$bytes)

Attempts to extract the next message in $bytes, returning the
bytes containing it, and deleting those bytes from the front
of $bytes.
Returns undef if no next message is available.

=cut

sub extract_message
{
    my ($self) = shift (@_);
    my ($bytes_ref) = shift (@_);
    my ($term_pos);

    ref ($bytes_ref) || return (undef);
    (($term_pos = index ($$bytes_ref, "\n")) == -1)
      && return undef;
    return (substr ($$bytes_ref, 0, $term_pos+1, ""));
};

=head2 ->decode ($bytes)

Decodes bytes in $bytes, and returns ($ok, $msg, $result)
where $ok == 1 and $result is the SATC message on success,
$ok == 0 and $msg is a human-readable error message on failure.

=cut

##  ($ok, $msg, $result) = decode ($bytes)
##    Returns in $result a hash representing the parsed SATC message
##    in $bytes.
sub decode
{
    my ($self) = shift (@_);
    my ($bytes) = shift (@_);
    my ($ok, $msg, $field_name, $conversion);
    my ($orig_bytes, $msgtype, $format, $field, $quotechar, $fields);

    ##  Have to decode SATC msg in $bytes
    if (length ($bytes) < 3)
    {
	return (0, "Cannot parse SATC message \"" . $bytes . "\": too short");
    };

    ##  First, remove message terminator byte(s)
    if (substr ($bytes, -2) eq "\r\n")
    {
	chop ($bytes);
	chop ($bytes);
    }
    elsif (substr ($bytes, -1) eq "\n")
    {
	chop ($bytes);
    };

    ##  Next, break it into fields and get the msgtype
    $orig_bytes = $bytes;
    (($ok, $msg, $fields) = _parse_SATC_fields ($bytes))[0]
      || return (0, "Cannot parse SATC message \"" . $orig_bytes . "\": $msg");
    scalar (@$fields)
      || return (0, "Ignoring empty SATC message \"" . $orig_bytes . "\"");
    defined ($msgtype = $self->{"msg_to_type"}{$fields->[0]})
      || return (0, "Unknown SATC command \"" . $fields->[0] .
		 "\" in message \"" . $orig_bytes . "\"");

    ##  Now build up hash representing the message
    defined ($format = $self->{"type_to_format"}{$msgtype})
      || return (0, "Unknown SATC format for \"" . $fields->[0] .
		 "\" in message \"" . $orig_bytes . "\"");
    $msg = {"msgtype" => $msgtype};
    shift (@$fields);
    foreach $field_name (@$format)
    {
	defined ($field = shift (@$fields))
	  || return (0,  "Insufficient fields in SATC message \"" .
		     $orig_bytes . "\"");
	if (defined ($conversion =
		     $self->{"field_to_decode_conv"}{$field_name}))
	{
	    $field = &$conversion ($field);
	};
	$msg->{$field_name} = $field;
    };
    return (1, "", $msg);
};

=head2 ->encode ($message)

Returns ($ok, $msg, $bytes) where $ok == 1 and $bytes is the
byte string of encoded SATC message $message on success,
$ok == 0 and $msg is a human-readable error message on failure.
The encoded byte string includes the terminating newline.
This method is forgiving (perhaps too forgiving) about missing
fields - it replaces missing string fields with empty strings
and missing integer fields with 0.
The msgtype field, however, is never allowed to be missing.

=cut

##  ($ok, $msg, $bytes) = encode ($message)
sub encode
{
    my ($self) = shift (@_);
    my ($msg) = shift (@_);
    my ($type, $format, $value, $field_name, $conversion, @encoded);
    my ($datatype);

    defined ($type = $msg->{"msgtype"})
      || return (0, "Undefined msgtype");

    if (! defined ($format = $self->{"type_to_format"}{$type}))
    {
	return (0, "Undefined format string message type: \"$type\"");
    };
    @encoded = $self->{"type_to_msg"}{$type};
    foreach $field_name (@$format)
    {
	$value = $msg->{$field_name};
	if (defined ($conversion =
		     $self->{"field_to_encode_conv"}{$field_name}))
	{
	    $value = &$conversion ($value);
	};
	if (! defined ($value))
	{
	    $datatype = $self->{"field_to_datatype"}{$field_name};
	    if ($datatype == DATATYPE_STRING)
	    {
		$value = "\"\"";
	    }
	    elsif ($datatype == DATATYPE_INT)
	    {
		$value = 0;
	    }
	    elsif ($datatype == DATATYPE_STRINGARRAYREF)
	    {
		$value = "\"\"";
	    };
	};
	push (@encoded, $value);
    };
    return (1, "", join (" ", @encoded) . "\n");
};


=head2 $hashed = hashify ($to_hash, $preserve_prefix_count, $suffix_blocks_count);

Given string to_hash, returns a hash string of max length
preserve_prefix_count + suffix_blocks_count*11
with preserve_prefix_count chars at beginning untouched,
and the remaining chars hashed into suffix_blocks_count*11
characters.

=cut

##  hashify
sub hashify
{
    my ($to_hash) = shift (@_);
    my ($preserve_prefix_count) = shift (@_);
    my ($suffix_blocks_count) = shift (@_);
    return _hashify ($to_hash, $preserve_prefix_count, $suffix_blocks_count);
};

=head1 AUTHOR

Doug Campbell, C<< <soup at ampersand.com> >>

=head1 BUGS

Please report any bugs or feature requests to
C<bug-satc at rt.cpan.org>, or through the web interface at
L<http://rt.cpan.org/NoAuth/ReportBug.html?Queue=Satc>.
I will be notified, and then you'll automatically be notified of progress on
your bug as I make changes.

=head1 SUPPORT

You can find documentation for this module with the perldoc command.

    perldoc Satc

=head1 ACKNOWLEDGEMENTS

=head1 COPYRIGHT & LICENSE

Copyright (C) 2006,2007 Ampersand Inc., and Doug Campbell

This file is part of Satc.

Satc is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Satc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Satc; if not, see <http://www.gnu.org/licenses/>.

=cut

bootstrap Satc $VERSION;        ##  if using xs

1; # End of Satc
