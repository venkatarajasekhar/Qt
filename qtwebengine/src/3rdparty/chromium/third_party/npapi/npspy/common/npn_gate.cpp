/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "xp.h"

#include "npapi.h"
#include "npupp.h"

#include "logger.h"

extern Logger * logger;
extern NPNetscapeFuncs NPNFuncs;

void NPN_Version(int* plugin_major, int* plugin_minor, int* netscape_major, int* netscape_minor)
{
  if(logger)
    logger->logCall(action_npn_version, (DWORD)plugin_major, (DWORD)plugin_minor, (DWORD)netscape_major, (DWORD)netscape_minor);

  *plugin_major   = NP_VERSION_MAJOR;
  *plugin_minor   = NP_VERSION_MINOR;
  *netscape_major = HIBYTE(NPNFuncs.version);
  *netscape_minor = LOBYTE(NPNFuncs.version);

  if(logger)
    logger->logReturn(action_npn_version);
}

NPError NPN_GetURLNotify(NPP instance, const char *url, const char *target, void* notifyData)
{
	int navMinorVers = NPNFuncs.version & 0xFF;
	
  NPError rv = NPERR_NO_ERROR;

  if(logger)
    logger->logCall(action_npn_get_url_notify, (DWORD)instance, (DWORD)url, (DWORD)target, (DWORD)notifyData);

  if( navMinorVers >= NPVERS_HAS_NOTIFICATION )
		rv = NPNFuncs.geturlnotify(instance, url, target, notifyData);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  if(logger)
    logger->logReturn(action_npn_get_url_notify, rv);

  return rv;
}

NPError NPN_GetURL(NPP instance, const char *url, const char *target)
{
  if(logger)
    logger->logCall(action_npn_get_url, (DWORD)instance, (DWORD)url, (DWORD)target);

  NPError rv = NPNFuncs.geturl(instance, url, target);

  if(logger)
    logger->logReturn(action_npn_get_url, rv);

  return rv;
}

NPError NPN_PostURLNotify(NPP instance, const char* url, const char* window, uint32 len, const char* buf, NPBool file, void* notifyData)
{
	int navMinorVers = NPNFuncs.version & 0xFF;

  NPError rv = NPERR_NO_ERROR;

  if(logger)
    logger->logCall(action_npn_post_url_notify, (DWORD)instance, (DWORD)url, (DWORD)window, (DWORD)len, (DWORD)buf, (DWORD)file, (DWORD)notifyData);

	if( navMinorVers >= NPVERS_HAS_NOTIFICATION )
		rv = NPNFuncs.posturlnotify(instance, url, window, len, buf, file, notifyData);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  if(logger)
    logger->logReturn(action_npn_post_url_notify, rv);

  return rv;
}

NPError NPN_PostURL(NPP instance, const char* url, const char* window, uint32 len, const char* buf, NPBool file)
{
  if(logger)
    logger->logCall(action_npn_post_url, (DWORD)instance, (DWORD)url, (DWORD)window, (DWORD)len, (DWORD)buf, (DWORD)file);

  NPError rv = NPNFuncs.posturl(instance, url, window, len, buf, file);

  if(logger)
    logger->logReturn(action_npn_post_url, rv);

  return rv;
} 

NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
  if(logger)
    logger->logCall(action_npn_request_read, (DWORD)stream, (DWORD)rangeList);

  NPError rv = NPNFuncs.requestread(stream, rangeList);

  if(logger)
    logger->logReturn(action_npn_request_read, rv);

  return rv;
}

NPError NPN_NewStream(NPP instance, NPMIMEType type, const char* target, NPStream** stream)
{
	int navMinorVersion = NPNFuncs.version & 0xFF;

  NPError rv = NPERR_NO_ERROR;

  if(logger)
    logger->logCall(action_npn_new_stream, (DWORD)instance, (DWORD)type, (DWORD)target, (DWORD)stream);

	if( navMinorVersion >= NPVERS_HAS_STREAMOUTPUT )
		rv = NPNFuncs.newstream(instance, type, target, stream);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  if(logger)
    logger->logReturn(action_npn_new_stream, rv);

  return rv;
}

int32 NPN_Write(NPP instance, NPStream *stream, int32 len, void *buffer)
{
	int navMinorVersion = NPNFuncs.version & 0xFF;

  int32 rv = 0;

  if(logger)
    logger->logCall(action_npn_write, (DWORD)instance, (DWORD)stream, (DWORD)len, (DWORD)buffer);

  if( navMinorVersion >= NPVERS_HAS_STREAMOUTPUT )
		rv = NPNFuncs.write(instance, stream, len, buffer);
	else
		rv = -1;

  if(logger)
    logger->logReturn(action_npn_write, rv);

  return rv;
}

NPError NPN_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
	int navMinorVersion = NPNFuncs.version & 0xFF;

  NPError rv = NPERR_NO_ERROR;

  if(logger)
    logger->logCall(action_npn_destroy_stream, (DWORD)instance, (DWORD)stream, (DWORD)reason);

  if( navMinorVersion >= NPVERS_HAS_STREAMOUTPUT )
		rv = NPNFuncs.destroystream(instance, stream, reason);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  if(logger)
    logger->logReturn(action_npn_destroy_stream, rv);

  return rv;
}

void NPN_Status(NPP instance, const char *message)
{
  if(logger)
    logger->logCall(action_npn_status, (DWORD)instance, (DWORD)message);

  NPNFuncs.status(instance, message);
}

const char* NPN_UserAgent(NPP instance)
{
  static const char * rv = NULL;

  if(logger)
    logger->logCall(action_npn_user_agent, (DWORD)instance);

  rv = NPNFuncs.uagent(instance);

  if(logger)
    logger->logReturn(action_npn_user_agent);

  return rv;
}

void* NPN_MemAlloc(uint32 size)
{
  void * rv = NULL;
  
  if(logger)
    logger->logCall(action_npn_mem_alloc, (DWORD)size);

  rv = NPNFuncs.memalloc(size);

  if(logger)
    logger->logReturn(action_npn_mem_alloc);

  return rv;
}

void NPN_MemFree(void* ptr)
{
  if(logger)
    logger->logCall(action_npn_mem_free, (DWORD)ptr);

  NPNFuncs.memfree(ptr);
}

uint32 NPN_MemFlush(uint32 size)
{
  if(logger)
    logger->logCall(action_npn_mem_flush, (DWORD)size);

  uint32 rv = NPNFuncs.memflush(size);

  if(logger)
    logger->logReturn(action_npn_mem_flush, rv);

  return rv;
}

void NPN_ReloadPlugins(NPBool reloadPages)
{
  if(logger)
    logger->logCall(action_npn_reload_plugins, (DWORD)reloadPages);

  NPNFuncs.reloadplugins(reloadPages);
}

#ifdef OJI
JRIEnv* NPN_GetJavaEnv(void)
{
  JRIEnv * rv = NULL;

  if(logger)
    logger->logCall(action_npn_get_java_env);

	rv = NPNFuncs.getJavaEnv();

  if(logger)
    logger->logReturn(action_npn_get_java_env);

  return rv;
}

jref NPN_GetJavaPeer(NPP instance)
{
  jref rv;

  if(logger)
    logger->logCall(action_npn_get_java_peer, (DWORD)instance);

	rv = NPNFuncs.getJavaPeer(instance);

  if(logger)
    logger->logReturn(action_npn_get_java_peer);

  return rv;
}
#else
void* NPN_GetJavaEnv(void)
{
  JRIEnv * rv = NULL;

  if(logger)
    logger->logCall(action_npn_get_java_env);

	rv = NULL;

  if(logger)
    logger->logReturn(action_npn_get_java_env);

  return rv;
}

void* NPN_GetJavaPeer(NPP instance)
{
  jref rv;

  if(logger)
    logger->logCall(action_npn_get_java_peer, (DWORD)instance);

	rv = NULL;

  if(logger)
    logger->logReturn(action_npn_get_java_peer);

  return rv;
}
#endif

NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value)
{
  NPError rv = NPERR_NO_ERROR;

  rv = NPNFuncs.getvalue(instance, variable, value);

  if(logger)
    logger->logCall(action_npn_get_value, (DWORD)instance, (DWORD)variable, (DWORD)value);

  if(logger)
    logger->logReturn(action_npn_get_value, rv);

  return rv;
}

NPError NPN_SetValue(NPP instance, NPPVariable variable, void *value)
{
  NPError rv = NPERR_NO_ERROR;

  if(logger)
    logger->logCall(action_npn_set_value, (DWORD)instance, (DWORD)variable, (DWORD)value);

  rv = NPNFuncs.setvalue(instance, variable, value);

  if(logger)
    logger->logReturn(action_npn_set_value, rv);

  return rv;
}

void NPN_InvalidateRect(NPP instance, NPRect *invalidRect)
{
  if(logger)
    logger->logCall(action_npn_invalidate_rect, (DWORD)instance, (DWORD)invalidRect);

  NPNFuncs.invalidaterect(instance, invalidRect);
}

void NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion)
{
  if(logger)
    logger->logCall(action_npn_invalidate_region, (DWORD)instance, (DWORD)invalidRegion);

  NPNFuncs.invalidateregion(instance, invalidRegion);
}

void NPN_ForceRedraw(NPP instance)
{
  if(logger)
    logger->logCall(action_npn_force_redraw, (DWORD)instance);

  NPNFuncs.forceredraw(instance);
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8 *name)
{
  if(logger)
    logger->logCall(action_npn_get_string_identifier, (DWORD)name);

  NPIdentifier rv = NPNFuncs.getstringidentifier(name);

  if(logger)
    logger->logReturn(action_npn_get_string_identifier, (DWORD)(rv));

  return rv;
}

bool NPN_Enumerate(NPP id, NPObject* obj, NPIdentifier** identifier, uint32_t*val)
{
  if(logger)
    logger->logCall(action_npn_enumerate);

  bool rv = NPNFuncs.enumerate(id, obj, identifier, val);

  if(logger)
    logger->logReturn(action_npn_enumerate, rv);

  return rv;
}

bool NPN_PopPopupsEnabledState(NPP id)
{
  if(logger)
    logger->logCall(action_npn_pop_popups_enabled_state, (DWORD)id);

  bool rv = NPNFuncs.poppopupsenabledstate(id);

  if(logger)
    logger->logReturn(action_npn_pop_popups_enabled_state, rv);

  return rv;
}

bool NPN_PushPopupsEnabledState(NPP id, NPBool enabled)
{
  if(logger)
    logger->logCall(action_npn_push_popups_enabled_state, (DWORD)id);

  bool rv = NPNFuncs.pushpopupsenabledstate(id, enabled);

  if(logger)
    logger->logReturn(action_npn_push_popups_enabled_state, rv);

  return rv;
}

void NPN_SetException(NPObject*obj, const NPUTF8*message)
{
  if(logger)
    logger->logCall(action_npn_set_exception, (DWORD)message);

  NPNFuncs.setexception(obj,message);
}

void NPN_ReleaseVariantValue(NPVariant*variant)
{
  if(logger)
    logger->logCall(action_npn_release_variant_value, (DWORD)variant);

  NPNFuncs.releasevariantvalue(variant);
}

bool NPN_HasMethod(NPP id, NPObject* object, NPIdentifier identifier)
{
  if(logger)
    logger->logCall(action_npn_has_method, (DWORD)identifier);

  bool rv = NPNFuncs.hasmethod(id, object, identifier);

  if(logger)
    logger->logReturn(action_npn_has_method, rv);

  return rv;
}

bool NPN_HasProperty(NPP id, NPObject* object, NPIdentifier identifier)
{
  if(logger)
    logger->logCall(action_npn_has_property, (DWORD)identifier);

  bool rv = NPNFuncs.hasmethod(id, object, identifier);

  if(logger)
    logger->logReturn(action_npn_has_property, rv);

  return rv;
}

bool NPN_RemoveProperty(NPP id, NPObject* object, NPIdentifier identifier)
{
  if(logger)
    logger->logCall(action_npn_remove_property, (DWORD)identifier);

  bool rv = NPNFuncs.hasmethod(id, object, identifier);

  return rv;
}

bool NPN_SetProperty(NPP id, NPObject* obj, NPIdentifier identifier, const NPVariant *variant)
{
  if(logger)
    logger->logCall(action_npn_set_property, (DWORD)identifier);

  bool rv = NPNFuncs.setproperty(id, obj, identifier, variant);

  return rv;
}

bool NPN_GetProperty(NPP id, NPObject* obj, NPIdentifier identifier, NPVariant *variant)
{
  if(logger)
    logger->logCall(action_npn_get_property, (DWORD)identifier);

  bool rv = NPNFuncs.getproperty(id, obj, identifier, variant);

  return rv;
}

bool NPN_Evaluate(NPP id, NPObject* obj, NPString* str, NPVariant* variant)
{
  if(logger)
    logger->logCall(action_npn_evaluate, (DWORD)str);

  bool rv = NPNFuncs.evaluate(id, obj, str, variant);

  if(logger)
    logger->logReturn(action_npn_evaluate, rv);

  return rv;
}

bool NPN_InvokeDefault(NPP id, NPObject* obj, const NPVariant* args, uint32_t count, NPVariant*result)
{
  if(logger)
    logger->logCall(action_npn_invoke_default, (DWORD)obj);

  bool rv = NPNFuncs.invokeDefault(id, obj, args, count, result);

  if(logger)
    logger->logReturn(action_npn_invoke_default, rv);

  return rv;
}

bool NPN_Invoke(NPP id, NPObject* obj, NPIdentifier identifier, const NPVariant *args, uint32_t count, NPVariant*result)
{
  if(logger)
    logger->logCall(action_npn_invoke, (DWORD)obj);

  bool rv = NPNFuncs.invoke(id, obj, identifier, args, count, result);

  if(logger)
    logger->logReturn(action_npn_invoke, rv);

  return rv;
}

void NPN_ReleaseObject(NPObject *obj) 
{
  if(logger)
    logger->logCall(action_npn_release_object, (DWORD)obj);

  NPNFuncs.releaseobject(obj);
}

NPObject *NPN_RetainObject(NPObject* obj)
{
  if(logger)
    logger->logCall(action_npn_retain_object, (DWORD)obj);

  NPObject *rv = NPNFuncs.retainobject(obj);

  return rv;
}

NPObject* NPN_CreateObject(NPP id, NPClass *cl)
{
  if(logger)
    logger->logCall(action_npn_create_object, (DWORD)cl);
  
  NPObject *rv = NPNFuncs.createobject(id, cl);

  if(logger)
    logger->logReturn(action_npn_create_object, (DWORD)rv);

  return rv;
}

int32_t NPN_IntFromIdentifier(NPIdentifier identifier)
{
  if(logger)
    logger->logCall(action_npn_int_from_identifier, (DWORD)identifier);

  int32_t rv = NPNFuncs.intfromidentifier(identifier);

  if(logger)
    logger->logReturn(action_npn_int_from_identifier, rv);

  return rv;
}

NPUTF8 *NPN_UTF8FromIdentifier(NPIdentifier identifier)
{
  if(logger)
    logger->logCall(action_npn_utf8_from_identifier, (DWORD)identifier);

  NPUTF8 *rv = NPNFuncs.utf8fromidentifier(identifier);

  if(logger)
    logger->logReturn(action_npn_utf8_from_identifier, 1234567890);

  return rv;
}

bool NPN_IdentifierIsString(NPIdentifier identifier)
{
  if(logger)
    logger->logCall(action_npn_identifier_is_string, (DWORD)identifier);

  bool rv = NPNFuncs.identifierisstring(identifier);

  if(logger)
    logger->logReturn(action_npn_identifier_is_string, rv);

  return rv;
}

NPIdentifier NPN_GetIntIdentifier(int32_t value)
{
  if(logger)
    logger->logCall(action_npn_get_int_identifer, (DWORD)value);

  NPIdentifier rv = NPNFuncs.getintidentifier(value);

  if(logger)
    logger->logReturn(action_npn_get_int_identifer, (DWORD)rv);

  return rv;
}

void NPN_GetStringIdentifiers(const NPUTF8 **names, int32_t count, NPIdentifier *identifiers)
{
  if(logger)
    logger->logCall(action_npn_get_string_identifiers);

  NPNFuncs.getstringidentifiers(names, count, identifiers);
}