/*
 * libjingle
 * Copyright 2011, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TALK_XMPP_IQTASK_H_
#define TALK_XMPP_IQTASK_H_

#include <string>

#include "talk/xmpp/xmpptask.h"
#include "talk/xmpp/xmppengine.h"

namespace buzz {

class IqTask : public XmppTask {
 public:
  IqTask(XmppTaskParentInterface* parent,
         const std::string& verb, const Jid& to,
         XmlElement* el);
  virtual ~IqTask() {}

  const XmlElement* stanza() const { return stanza_.get(); }

  sigslot::signal2<IqTask*,
                   const XmlElement*> SignalError;

 protected:
  virtual void HandleResult(const XmlElement* element) = 0;

 private:
  virtual int ProcessStart();
  virtual bool HandleStanza(const XmlElement* stanza);
  virtual int ProcessResponse();
  virtual int OnTimeout();

  Jid to_;
  talk_base::scoped_ptr<XmlElement> stanza_;
};

}  // namespace buzz

#endif  // TALK_XMPP_IQTASK_H_
