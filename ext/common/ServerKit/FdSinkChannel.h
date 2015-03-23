/*
 *  Phusion Passenger - https://www.phusionpassenger.com/
 *  Copyright (c) 2014-2015 Phusion
 *
 *  "Phusion Passenger" is a trademark of Hongli Lai & Ninh Bui.
 *
 *  See LICENSE file for license information.
 */
#ifndef _PASSENGER_SERVER_KIT_FD_SINK_CHANNEL_H_
#define _PASSENGER_SERVER_KIT_FD_SINK_CHANNEL_H_

#include <oxt/macros.hpp>
#include <cerrno>
#include <unistd.h>
#include <ev.h>
#include <ServerKit/Channel.h>
#include <Utils/json.h>

namespace Passenger {
namespace ServerKit {

using namespace oxt;


class FdSinkChannel: protected Channel {
private:
	ev_io watcher;

	static Result _onData(Channel *channel, const MemoryKit::mbuf &buffer, int errcode) {
		return static_cast<FdSinkChannel *>(channel)->onData(buffer, errcode);
	}

	Result onData(const MemoryKit::mbuf &buffer, int errcode) {
		if (buffer.size() > 0) {
			// Data
			ssize_t ret;

			do {
				ret = ::write(watcher.fd, buffer.start, buffer.size());
			} while (ret == -1 && errno == EAGAIN);
			if (ret == (ssize_t) buffer.size()) {
				return Result(ret, false);
			} else if (ret >= 0) {
				ev_io_start(ctx->libev->getLoop(), &watcher);
				stop();
				return Result(ret, false);
			} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
				ev_io_start(ctx->libev->getLoop(), &watcher);
				stop();
				return Result(0, false);
			} else {
				Channel::feedError(errno);
				return Result(0, false);
			}
		} else if (errcode == 0) {
			// EOF
			return Channel::Result(0, true);
		} else {
			// Error
			// We do nothing here. The caller is responsible for handling the error.
			return Channel::Result(0, false);
		}
	}

	static void _onWritable(EV_P_ ev_io *io, int revents) {
		FdSinkChannel *self = static_cast<FdSinkChannel *>(io->data);
		ev_io_stop(self->ctx->libev->getLoop(), &self->watcher);
		self->start();
	}

	void initialize() {
		dataCallback = _onData;
		watcher.active = false;
		watcher.fd = -1;
		watcher.data = this;
	}

public:
	FdSinkChannel() {
		initialize();
	}

	FdSinkChannel(Context *context)
		: Channel(context)
	{
		initialize();
	}

	~FdSinkChannel() {
		if (ctx != NULL && ev_is_active(&watcher)) {
			ev_io_stop(ctx->libev->getLoop(), &watcher);
		}
	}

	// May only be called right after construction.
	OXT_FORCE_INLINE
	void setContext(Context *context) {
		Channel::setContext(context);
	}

	void reinitialize(int fd) {
		Channel::reinitialize();
		ev_io_init(&watcher, _onWritable, fd, EV_WRITE);
	}

	void deinitialize() {
		if (ev_is_active(&watcher)) {
			ev_io_stop(ctx->libev->getLoop(), &watcher);
		}
		watcher.fd = -1;
		Channel::deinitialize();
	}

	OXT_FORCE_INLINE
	int feed(const MemoryKit::mbuf &mbuf) {
		return Channel::feed(mbuf);
	}

	OXT_FORCE_INLINE
	int feed(BOOST_RV_REF(MemoryKit::mbuf) mbuf) {
		return Channel::feed(mbuf);
	}

	OXT_FORCE_INLINE
	int feedWithoutRefGuard(const MemoryKit::mbuf &mbuf) {
		return Channel::feedWithoutRefGuard(mbuf);
	}

	OXT_FORCE_INLINE
	int feedWithoutRefGuard(BOOST_RV_REF(MemoryKit::mbuf) mbuf) {
		return Channel::feedWithoutRefGuard(mbuf);
	}

	OXT_FORCE_INLINE
	void feedError(int errcode) {
		return Channel::feedError(errcode);
	}

	OXT_FORCE_INLINE
	int getFd() const {
		return watcher.fd;
	}

	OXT_FORCE_INLINE
	bool acceptingInput() const {
		return Channel::acceptingInput();
	}

	OXT_FORCE_INLINE
	bool mayAcceptInputLater() const {
		return Channel::mayAcceptInputLater();
	}

	OXT_FORCE_INLINE
	bool hasError() const {
		return Channel::hasError();
	}

	OXT_FORCE_INLINE
	int getErrcode() const {
		return Channel::getErrcode();
	}

	OXT_FORCE_INLINE
	bool ended() const {
		return Channel::ended();
	}

	OXT_FORCE_INLINE
	bool endAcked() const {
		return Channel::endAcked();
	}

	OXT_FORCE_INLINE
	void setConsumedCallback(ConsumedCallback callback) {
		Channel::consumedCallback = callback;
	}

	OXT_FORCE_INLINE
	Hooks *getHooks() const {
		return hooks;
	}

	OXT_FORCE_INLINE
	void setHooks(Hooks *hooks) {
		this->hooks = hooks;
	}

	Json::Value inspectAsJson() const {
		Json::Value doc = Channel::inspectAsJson();
		doc["initialized"] = watcher.fd != -1;
		doc["io_watcher_active"] = (bool) watcher.active;
		return doc;
	}
};


} // namespace ServerKit
} // namespace Passenger

#endif /* _PASSENGER_SERVER_KIT_FD_SINK_CHANNEL_H_ */
