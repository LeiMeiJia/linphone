/*
 * cpim-chat-message-modifier.cpp
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cpim-chat-message-modifier.h"

#include "chat/cpim/cpim.h"
#include "content/content-type.h"
#include "content/content.h"
#include "address/address.h"
#include "logger/logger.h"
#include "chat/chat-message-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

int CpimChatMessageModifier::encode (ChatMessagePrivate *messagePrivate) {
	Cpim::Message message;
	Cpim::GenericHeader cpimContentTypeHeader;
	cpimContentTypeHeader.setName("Content-Type");
	cpimContentTypeHeader.setValue("Message/CPIM");
	message.addCpimHeader(cpimContentTypeHeader);

	Content content;
	if (!messagePrivate->internalContent.isEmpty()) {
		// Another ChatMessageModifier was called before this one, we apply our changes on the private content
		content = messagePrivate->internalContent;
	} else {
		// We're the first ChatMessageModifier to be called, we'll create the private content from the public one
		// We take the first one because if there is more of them, the multipart modifier should have been called first
		// So we should not be in this block
		content = messagePrivate->contents.front();
	}

	string contentType = content.getContentType().asString();
	const vector<char> body = content.getBody();
	string contentBody(body.begin(), body.end());

	Cpim::GenericHeader contentTypeHeader;
	contentTypeHeader.setName("Content-Type");
	contentTypeHeader.setValue(contentType);
	message.addContentHeader(contentTypeHeader);

	message.setContent(contentBody);

	if (!message.isValid()) {
		lError() << "[CPIM] Message is invalid: " << contentBody;
		return 500;
	} else {
		Content newContent;
		ContentType newContentType("Message/CPIM");
		newContent.setContentType(newContentType);
		newContent.setBody(message.asString());
		messagePrivate->internalContent = newContent;
	}
	return 0;
}

int CpimChatMessageModifier::decode (ChatMessagePrivate *messagePrivate) {
	Content content;
	if (!messagePrivate->internalContent.isEmpty()) {
		content = messagePrivate->internalContent;
	} else {
		content = messagePrivate->contents.front();
	}

	if (content.getContentType() == ContentType::Cpim) {
		const vector<char> body = content.getBody();
		string contentBody(body.begin(), body.end());
		shared_ptr<const Cpim::Message> message = Cpim::Message::createFromString(contentBody);
		if (message && message->isValid()) {
			Content newContent;
			ContentType newContentType(message->getContentHeaders()->front()->getValue());
			newContent.setContentType(newContentType);
			newContent.setBody(message->getContent());
			messagePrivate->internalContent = newContent;
		} else {
			lError() << "[CPIM] Message is invalid: " << contentBody;
			return 500;
		}
	} else {
		lError() << "[CPIM] Message is not CPIM but " << content.getContentType().asString();
		return -1;
	}
	return 0;
}

LINPHONE_END_NAMESPACE
