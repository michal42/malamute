/*  =========================================================================
    mlm_mailbox_simple - simple mailbox engine

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    This class implements a simple mailbox engine and serves as a basis for
    your own, more sophisticated mailbox engines. Mailbox engines are built
    as CZMQ actors.
@discuss

@end
*/

#include "mlm_classes.h"

//  --------------------------------------------------------------------------
//  The self_t structure holds the state for one actor instance

typedef struct {
    zsock_t *pipe;              //  Actor command pipe
    zpoller_t *poller;          //  Socket poller
    bool terminated;            //  Did caller ask us to quit?
    bool verbose;               //  Verbose logging enabled?
    zhashx_t *mailboxes;        //  Mailboxes as queues of messages
} self_t;

static void
s_self_destroy (self_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        self_t *self = *self_p;
        zpoller_destroy (&self->poller);
        zhashx_destroy (&self->mailboxes);
        free (self);
        *self_p = NULL;
    }
}

static self_t *
s_self_new (zsock_t *pipe)
{
    self_t *self = (self_t *) zmalloc (sizeof (self_t));
    if (self) {
        self->pipe = pipe;
        self->poller = zpoller_new (self->pipe, NULL);
        if (self->poller)
            self->mailboxes = zhashx_new ();
        if (self->mailboxes)
            zhashx_set_destructor (self->mailboxes, (czmq_destructor *) zlistx_destroy);
        else
            s_self_destroy (&self);
    }
    return self;
}

    
//  --------------------------------------------------------------------------
//  Here we implement the mailbox API

static int
s_self_handle_command (self_t *self)
{
    char *method = zstr_recv (self->pipe);
    if (!method)
        return -1;              //  Interrupted; exit zloop
    if (self->verbose)
        zsys_debug ("mlm_mailbox_simple: API command=%s", method);

    if (streq (method, "VERBOSE"))
        self->verbose = true;       //  Start verbose logging
    else
    if (streq (method, "$TERM"))
        self->terminated = true;    //  Shutdown the engine
    else
    if (streq (method, "STORE")) {
        char *mailbox;
        mlm_msg_t *msg;
        zsock_recv (self->pipe, "sp", &mailbox, &msg);
        zlistx_t *queue = (zlistx_t *) zhashx_lookup (self->mailboxes, mailbox);
        if (!queue) {
            queue = zlistx_new ();
            zhashx_insert (self->mailboxes, mailbox, queue);
            zlistx_set_destructor (queue, (czmq_destructor *) mlm_msg_destroy);
        }
        zlistx_add_end (queue, msg);
        zstr_free (&mailbox);
    }
    else
    if (streq (method, "QUERY")) {
        char *mailbox;
        mlm_msg_t *msg = NULL;
        zsock_recv (self->pipe, "s", &mailbox);
        zlistx_t *queue = (zlistx_t *) zhashx_lookup (self->mailboxes, mailbox);
        if (queue && zlistx_size (queue))
            msg = (mlm_msg_t *) zlistx_detach (queue, NULL);
        zsock_send (self->pipe, "p", msg);
        zstr_free (&mailbox);
    }
    //  Cleanup pipe if any argument frames are still waiting to be eaten
    if (zsock_rcvmore (self->pipe)) {
        zsys_error ("mlm_mailbox_simple: trailing API command frames (%s)", method);
        zmsg_t *more = zmsg_recv (self->pipe);
        zmsg_print (more);
        zmsg_destroy (&more);
    }
    zstr_free (&method);
    return self->terminated? -1: 0;
}


static void mailbox_mem_debug(self_t *self, time_t ts)
{
	struct tm *tm;
	char fmt[256], fname[256];
	FILE *f;

	mkdir("/var/lib/fty/mlm-debug", 0777);
	snprintf(fmt, sizeof(fmt), "/var/lib/fty/mlm-debug/mailbox-%%Y-%%m-%%dT%%H:%%M:%%S.%d.txt", getpid());
	tm = gmtime(&ts);
	strftime(fname, sizeof(fname), fmt, tm);

	zsys_warning("mailbox_mem_debug -> %s", fname);
	f = fopen(fname, "w");
	if (!f) {
		zsys_error("error writing to %s: %s", fname, strerror(errno));
		return;
	}

	fprintf(f, "# of mailbox queues: %zd\n", zhashx_size(self->mailboxes));
	zlistx_t *keys = zhashx_keys (self->mailboxes);
	char *key = (char *) zlistx_first (keys);
	while (key) {
		zlistx_t *queue = (zlistx_t *)zhashx_lookup(self->mailboxes, key);
		size_t i, size = zlistx_size(queue);
		fprintf(f, "queue %s: %zd msgs\n", key, size);
		mlm_msg_t *msg = (mlm_msg_t *)zlistx_first(queue);
		bool first = true, elipsis = false;
		for (i = 0; msg; i++, msg = (mlm_msg_t *)zlistx_next(queue)) {
			/* Only print first 3 and last 3 messages */
			if (i >= 3 && size - i > 3) {
				if (!elipsis)
					fputs(", [...]", f);
				elipsis = true;
				continue;
			}
			if (first)
				fputs("\t", f);
			else
				fputs(", ", f);
			first = false;
			fprintf(f, "%s/%s", mlm_msg_sender(msg),
					    mlm_msg_subject(msg));
		}
		if (!first)
			fputs("\n", f);
		key = (char *) zlistx_next (keys);
	}
	zlistx_destroy (&keys);

	fclose(f);
	zsys_warning("mailbox_mem_debug complete %s", fname);
}

//  --------------------------------------------------------------------------
//  This method implements the mlm_mailbox_simple actor interface

void
mlm_mailbox_simple (zsock_t *pipe, void *args)
{
	static time_t last_timestamp;
    self_t *self = s_self_new (pipe);
    //  Signal successful initialization
    zsock_signal (pipe, 0);

    while (!self->terminated) {
        zsock_t *which = (zsock_t *) zpoller_wait (self->poller, -1);
        if (which == self->pipe)
            s_self_handle_command (self);
        else
        if (zpoller_terminated (self->poller))
            break;          //  Interrupted

	time_t now = time(NULL);

	if (now - last_timestamp >= 60) {
		last_timestamp = now;
		mailbox_mem_debug(self, now);
	}
    }
    s_self_destroy (&self);
}


//  --------------------------------------------------------------------------
//  Selftest

void
mlm_mailbox_simple_test (bool verbose)
{
    printf (" * mlm_mailbox_simple: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    zactor_t *mailbox = zactor_new (mlm_mailbox_simple, NULL);
    assert (mailbox);
    if (verbose)
        zstr_sendx (mailbox, "VERBOSE", NULL);

    zactor_destroy (&mailbox);
    //  @end
    printf ("OK\n");
}
