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

/*
 * Poll
 */

value
caml_iomux_poll(value v_fds, value v_nfds, value v_timo)
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
		uerror("poll", Nothing);

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

#define S_IN_NS 1000000000LL
value
caml_iomux_ppoll(value v_fds, value v_nfds, value v_timo, value v_sigmask)
{
	CAMLparam4(v_fds, v_nfds, v_timo, v_sigmask);
	struct pollfd *fds;
	struct timespec *timo;
	struct timespec ts;
	sigset_t *psigmask, sigmask;
	nfds_t nfds;
	int64_t timo64;
	int r;

	fds = Caml_ba_data_val(v_fds);
	nfds = Int_val(v_nfds);
	timo64 = Int64_val(v_timo);
	if (timo64 == -1LL)
		timo = NULL;
	else {
		ts.tv_sec = (time_t)(timo64 / S_IN_NS);
		ts.tv_nsec = (time_t)(timo64 % S_IN_NS);
		timo = &ts;
	}

	if (v_sigmask == Val_emptylist)
		psigmask = NULL;
	else {
		decode_sigset(v_sigmask, &sigmask);
		psigmask = &sigmask;
	}
#if 0
	for (int i = 0; i < nfds; i++) {
		printf("[%d] fd = %d events=0x%x (nfds=%d)\n",
		    i, fds[i].fd, fds[i].events, nfds);
	}
#endif
	caml_enter_blocking_section();
	r = ppoll(fds, nfds, timo, psigmask);
	caml_leave_blocking_section();
	if (r == -1) /* this allocs */
		uerror("poll", Nothing);

#if 0
	printf("r=%d\n", r);
	for (int i = 0; i < 10; i++) {
		printf("[%d] fd = %d events=0x%x revents=0x%x(nfds=%d)\n",
		    i, fds[i].fd, fds[i].events, fds[i].revents, nfds);
	}
#endif

	CAMLreturn(Val_int(r));
}
#undef S_IN_NS

#define pollfd_of_index(vfds, vindex)					\
	((struct pollfd *)Caml_ba_data_val(vfds) + (Int_val (v_index)))

value /* noalloc */
caml_iomux_poll_set_index(value v_fds, value v_index, value v_fd, value v_events)
{
	struct pollfd *pfd = pollfd_of_index(v_fds, v_index);

	pfd->fd = Int_val(v_fd);
	pfd->events = Int_val(v_events);

	return (Val_unit);
}

value /* noalloc */
caml_iomux_poll_get_revents(value v_fds, value v_index)
{
	struct pollfd *pfd = pollfd_of_index(v_fds, v_index);

	return (Val_int(pfd->revents));
}

value /* noalloc */
caml_iomux_poll_get_fd(value v_fds, value v_index)
{
	struct pollfd *pfd = pollfd_of_index(v_fds, v_index);

	return (Val_int(pfd->fd));
}

/*
 * Util
 */

value /* noalloc */
caml_iomux_poll_max_open_files(value v_unit)
{
	return (Val_int(sysconf(_SC_OPEN_MAX)));
}
