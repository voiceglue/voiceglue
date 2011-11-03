//  Copyright 2006,2007 Ampersand Inc., Doug Campbell
//
//  This file is part of libvglue.
//
//  libvglue is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  libvglue is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with libvglue; if not, see <http://www.gnu.org/licenses/>.

#ifndef VGLUE_RUN_H
#define VGLUE_RUN_H

#ifdef __cplusplus
extern "C" {
#endif

int voiceglue_start_platform (int num_channels, int logfd, int loglevel,
			      void **platformHandle);
void voiceglue_free_parse_tree (const char *addr);
int voiceglue_start_thread (void *platformHandle,
			    int callid, int ipcfd, char *url,
			    char *ani, char *dnis,
			    char *javascript_init,
			    void **channelHandle);
int voiceglue_stop_thread (void *channel_handle);
int voiceglue_stop_platform (void *platform_handle);
void voiceglue_set_trace (int trace_state);

#ifdef __cplusplus
}
#endif

#endif /* include guard VGLUE_RUN_H */
