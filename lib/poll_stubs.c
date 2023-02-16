#define _GNU_SOURCE

#include <poll.h>
#include <signal.h>

#include <caml/bigarray.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <caml/unixsupport.h>
#include <caml/signals.h>

/* only defined in the runtime with CAML_INTERNALS */
CAMLextern int caml_convert_signal_number (int);

value
caml_poll2(value v_fds, value v_nfds, value v_timo)
{
	CAMLparam3(v_fds, v_nfds, v_timo);
	struct pollfd *fds;
	nfds_t nfds;
	int timo;
	int r;

	fds = Caml_ba_data_val(v_fds);
	nfds = Int_val(v_nfds);
	timo = Int_val(v_timo);
#if 0
	for (int i = 0; i < nfds; i++) {
		printf("[%d] fd = %d events=0x%x (nfds=%d)\n",
		    i, fds[i].fd, fds[i].events, nfds);
	}
#endif
	caml_enter_blocking_section();
	r = poll(fds, nfds, timo);
	caml_leave_blocking_section();
	if (r == -1) /* this allocs */
		caml_uerror("poll", Nothing);

#if 0
	printf("r=%d\n", r);
	for (int i = 0; i < 10; i++) {
		printf("[%d] fd = %d events=0x%x revents=0x%x(nfds=%d)\n",
		    i, fds[i].fd, fds[i].events, fds[i].revents, nfds);
	}
#endif

	CAMLreturn(Val_int(r));
}

static void
decode_sigset(value vset, sigset_t * set)
{
	sigemptyset(set);
	for (/*nothing*/; vset != Val_emptylist; vset = Field(vset, 1)) {
		int sig = caml_convert_signal_number(Int_val(Field(vset, 0)));
		sigaddset(set, sig);
	}
}

value
caml_ppoll2(value v_fds, value v_nfds, value v_timo, value v_sigmask)
{
	CAMLparam4(v_fds, v_nfds, v_timo, v_sigmask);
	struct pollfd *fds;
	struct timespec *timo;
	struct timespec ts;
	sigset_t sigmask;
	nfds_t nfds;
	double d;
	int r;

	fds = Caml_ba_data_val(v_fds);
	nfds = Int_val(v_nfds);
	d = Double_val(v_timo);
	if (d == -1.0)
		timo = NULL;
	else {
		ts.tv_sec = (time_t) d;
		ts.tv_nsec = (d - ts.tv_sec) * 1e9;
		timo = &ts;
	}

	decode_sigset(v_sigmask, &sigmask);
#if 0
	for (int i = 0; i < nfds; i++) {
		printf("[%d] fd = %d events=0x%x (nfds=%d)\n",
		    i, fds[i].fd, fds[i].events, nfds);
	}
#endif
	caml_enter_blocking_section();
	r = ppoll(fds, nfds, timo, &sigmask);
	caml_leave_blocking_section();
	if (r == -1) /* this allocs */
		caml_uerror("poll", Nothing);

#if 0
	printf("r=%d\n", r);
	for (int i = 0; i < 10; i++) {
		printf("[%d] fd = %d events=0x%x revents=0x%x(nfds=%d)\n",
		    i, fds[i].fd, fds[i].events, fds[i].revents, nfds);
	}
#endif

	CAMLreturn(Val_int(r));
}

#define pollfd_of_index(vfds, vindex)				\
	(Caml_ba_data_val(vfds) +				\
	    (sizeof(struct pollfd) * (Int_val (v_index))))

value /* noalloc */
caml_poll2_set_index(value v_fds, value v_index, value v_fd, value v_events)
{
	struct pollfd *pfd = pollfd_of_index(v_fds, v_index);

	pfd->fd = Int_val(v_fd);
	pfd->events = Int_val(v_events);

	return (Val_unit);
}

value /* noalloc */
caml_poll2_get_revents(value v_fds, value v_index)
{
	struct pollfd *pfd = pollfd_of_index(v_fds, v_index);

	return (Val_int(pfd->revents));
}

value /* noalloc */
caml_poll2_get_fd(value v_fds, value v_index)
{
	struct pollfd *pfd = pollfd_of_index(v_fds, v_index);

	return (Val_int(pfd->fd));
}

value /* noalloc */
caml_poll2_max_open_files(value v_unit)
{
	return (Val_int(sysconf(_SC_OPEN_MAX)));
}
