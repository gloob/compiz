/*
 * Compiz configuration system library
 *
 * Copyright (C) 2007  Dennis Kasprzyk <onestone@opencompositing.org>
 * Copyright (C) 2007  Danny Baumann <maniac@opencompositing.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef CCS_BACKEND_H
#define CCS_BACKEND_H

#include <ccs-object.h>
#include <ccs-string.h>
#include <ccs-list.h>

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSSetting	  CCSSetting;
typedef struct _CCSPlugin         CCSPlugin;
typedef struct _CCSContext	  CCSContext;
typedef struct _CCSBackend	  CCSBackend;
typedef struct _CCSBackendInfo    CCSBackendInfo;
typedef struct _CCSBackendPrivate CCSBackendPrivate;
typedef struct _CCSBackendInterface  CCSBackendInterface;

typedef struct _CCSIntegration CCSIntegration;
typedef struct _CCSIntegrationInterface CCSIntegrationInterface;

typedef int (*CCSIntegrationGetIntegratedOptionIndex) (CCSIntegration *integration,
							      const char *pluginName,
							      const char *settingName);
typedef Bool (*CCSIntegrationReadOptionIntoSetting) (CCSIntegration *integration,
							    CCSContext		  *context,
							    CCSSetting		  *setting,
							    int			  index);
typedef void (*CCSIntegrationWriteSettingIntoOption) (CCSIntegration *integration,
							     CCSContext		   *context,
							     CCSSetting		   *setting,
							     int		   index);
typedef void (*CCSFreeIntegrationBackend) (CCSIntegration *integration);

struct _CCSIntegrationInterface
{
    CCSIntegrationGetIntegratedOptionIndex getIntegratedOptionIndex;
    CCSIntegrationReadOptionIntoSetting readOptionIntoSetting;
    CCSIntegrationWriteSettingIntoOption writeSettingIntoOption;
    CCSFreeIntegrationBackend			freeIntegrationBackend;
};

/**
 * @brief The _CCSIntegration struct
 *
 * An object that represents integration with a desktop environment. Generally
 * these objects store a list of hardcoded settings that can be retrieved using
 * ccsIntegrationGetIntegratedOptionIndex and then written to and read
 * from using the readOptionIntoSetting and writeOptionIntoSetting.
 */
struct _CCSIntegration
{
    CCSObject object;
};

CCSREF_HDR (Integration, CCSIntegration)

unsigned int ccsCCSIntegrationInterfaceGetType ();

int ccsIntegrationGetIntegratedOptionIndex (CCSIntegration *integration,
						   const char *pluginName,
						   const char *settingName);
Bool ccsIntegrationReadOptionIntoSetting (CCSIntegration *integration,
						 CCSContext		  *context,
						 CCSSetting		  *setting,
						 int			  index);
void ccsIntegrationWriteSettingIntoOption (CCSIntegration *integration,
						  CCSContext		   *context,
						  CCSSetting		   *setting,
						  int			    index);

void ccsFreeIntegration (CCSIntegration *integration);

CCSIntegration *
ccsNullIntegrationBackendNew (CCSObjectAllocationInterface *ai);

/**
 * @brief CCSBackend
 *
 * This object represents CCSBackend directly in memory. It does
 * not represent the details that come with a loaded backend.
 *
 * Backends are capable of reading, writing and updating settings
 * from intergrated locations. Clients should check if these functions
 * are supported first as the function pointers are permitted to be NULL.
 */
struct _CCSBackend
{
    CCSObject        object;
};

struct _CCSBackendInfo
{
    const char *name;              /* name of the backend */
    const char *shortDesc;         /* backend's short description */
    const char *longDesc;          /* backend's long description */
    Bool integrationSupport; /* does the backend support DE integration? */
    Bool profileSupport;     /* does the backend support profiles? */
    unsigned int refCount;   /* reference count */
};

typedef CCSBackendInterface * (*BackendGetInfoProc) (void);

typedef void (*CCSBackendExecuteEventsFunc) (CCSBackend *backend, unsigned int flags);

typedef Bool (*CCSBackendInitFunc) (CCSBackend *, CCSContext * context);
typedef Bool (*CCSBackendFiniFunc) (CCSBackend *);

typedef Bool (*CCSBackendReadInitFunc) (CCSBackend *, CCSContext * context);
typedef void (*CCSBackendReadSettingFunc)
(CCSBackend *, CCSContext * context, CCSSetting * setting);
typedef void (*CCSBackendReadDoneFunc) (CCSBackend *backend, CCSContext * context);

typedef Bool (*CCSBackendWriteInitFunc) (CCSBackend *backend, CCSContext * context);
typedef void (*CCSBackendWriteSettingFunc)
(CCSBackend *, CCSContext * context, CCSSetting * setting);
typedef void (*CCSBackendWriteDoneFunc) (CCSBackend *, CCSContext * context);

typedef void (*CCSBackendUpdateFunc) (CCSBackend *, CCSContext *, CCSPlugin *, CCSSetting *);

typedef Bool (*CCSBackendGetSettingIsIntegratedFunc) (CCSBackend *, CCSSetting * setting);
typedef Bool (*CCSBackendGetSettingIsReadOnlyFunc) (CCSBackend *, CCSSetting * setting);

typedef CCSStringList (*CCSBackendGetExistingProfilesFunc) (CCSBackend *, CCSContext * context);
typedef Bool (*CCSBackendDeleteProfileFunc) (CCSBackend *, CCSContext * context, char * name);

typedef void (*CCSBackendSetIntegration) (CCSBackend *, CCSIntegration *);

typedef const CCSBackendInfo * (*CCSBackendGetInfoFunc) (CCSBackend *);

struct _CCSBackendInterface
{
    CCSBackendGetInfoFunc      backendGetInfo;

    /* something like a event loop call for the backend,
       so it can check for file changes (gconf changes in the gconf backend)
       no need for reload settings signals anymore */
    CCSBackendExecuteEventsFunc executeEvents;

    CCSBackendInitFunc	       backendInit;
    CCSBackendFiniFunc	       backendFini;

    CCSBackendReadInitFunc     readInit;
    CCSBackendReadSettingFunc  readSetting;
    CCSBackendReadDoneFunc     readDone;

    CCSBackendWriteInitFunc    writeInit;
    CCSBackendWriteSettingFunc writeSetting;
    CCSBackendWriteDoneFunc    writeDone;

    CCSBackendUpdateFunc       updateSetting;

    CCSBackendGetSettingIsIntegratedFunc     getSettingIsIntegrated;
    CCSBackendGetSettingIsReadOnlyFunc       getSettingIsReadOnly;

    CCSBackendGetExistingProfilesFunc getExistingProfiles;
    CCSBackendDeleteProfileFunc       deleteProfile;
    CCSBackendSetIntegration	      setIntegration;
};

unsigned int ccsCCSBackendInterfaceGetType ();

/**
 * @brief ccsBackendGetInfo
 * @param backend a CCSBackend *
 * @return a const CCSBackendInfo * for this backend
 *
 * This function returns some basic info about this backend, what its
 * name is, what it suppoirts etc
 */
const CCSBackendInfo * ccsBackendGetInfo (CCSBackend *backend);

/**
 * @brief ccsBackendExecuteEvents
 * @param backend a CCSBackend *
 * @param flags ProcessEventsGlibMainLoopMask or 0
 *
 * something like a event loop call for the backend,
 * so it can check for file changes (gconf changes in the gconf backend)
 */
void ccsBackendExecuteEvents (CCSBackend *backend, unsigned int flags);

/**
 * @brief ccsBackendInit
 * @param backend
 * @param context
 * @return
 *
 * Initializes a backend for a context
 */
Bool ccsBackendInit (CCSBackend *backend, CCSContext *context);

/**
 * @brief ccsBackendFini
 * @param backend
 * @param context
 * @return
 *
 * Cleans up the backend
 */
Bool ccsBackendFini (CCSBackend *backend);
Bool ccsBackendReadInit (CCSBackend *backend, CCSContext *context);
void ccsBackendReadSetting (CCSBackend *backend, CCSContext *context, CCSSetting *setting);
void ccsBackendReadDone (CCSBackend *backend, CCSContext *context);
Bool ccsBackendWriteInit (CCSBackend *backend, CCSContext *context);
void ccsBackendWriteSetting (CCSBackend *backend, CCSContext *context, CCSSetting *setting);
void ccsBackendWriteDone (CCSBackend *backend, CCSContext *context);

/**
 * @brief ccsBackendUpdateSetting
 * @param backend The backend on which the update should be processed
 * @param context The context on which the backend resides.
 * @param plugin The plugin for the setting
 * @param setting The setting itself.
 *
 * This causes the specified setting to be re-read from the configuration
 * database and re-written to any integrated keys. It should genrally
 * be called by calback functions which know that the value
 * has changed.
 */
void ccsBackendUpdateSetting (CCSBackend *backend, CCSContext *context, CCSPlugin *plugin, CCSSetting *setting);
Bool ccsBackendGetSettingIsIntegrated (CCSBackend *backend, CCSSetting *setting);
Bool ccsBackendGetSettingIsReadOnly (CCSBackend *backend, CCSSetting *setting);

/**
 * @brief ccsBackendGetExistingProfiles
 * @param backend
 * @param context
 * @return a CCSStringist of available profiles for this backend
 */
CCSStringList ccsBackendGetExistingProfiles (CCSBackend *backend, CCSContext *context);
Bool ccsBackendDeleteProfile (CCSBackend *backend, CCSContext *context, char *name);
void ccsBackendSetIntegration (CCSBackend *backend, CCSIntegration *integration);
void ccsFreeBackend (CCSBackend *backend);

typedef struct _CCSDynamicBackend	  CCSDynamicBackend;
typedef struct _CCSDynamicBackendPrivate CCSDynamicBackendPrivate;
typedef struct _CCSDynamicBackendInterface  CCSDynamicBackendInterface;
typedef struct _CCSInterfaceTable         CCSInterfaceTable;

/**
 * @brief The _CCSDynamicBackend struct
 *
 * This object represents a CCSBackend loaded in memory as a dlopen
 * object. It implements the CCSBackend interface and provides an
 * interface of its own for managing the dynamic backend and checking
 * its capabilities.
 *
 * All function pointers are fully implemented and are safe to call
 */
struct _CCSDynamicBackend
{
    CCSObject object;
};

typedef const char * (*CCSDynamicBackendGetBackendName) (CCSDynamicBackend *);
typedef Bool (*CCSDynamicBackendSupportsRead) (CCSDynamicBackend *);
typedef Bool (*CCSDynamicBackendSupportsWrite) (CCSDynamicBackend *);
typedef Bool (*CCSDynamicBackendSupportsProfiles) (CCSDynamicBackend *);
typedef Bool (*CCSDynamicBackendSupportsIntegration) (CCSDynamicBackend *);
typedef CCSBackend * (*CCSDynamicBackendGetRawBackend) (CCSDynamicBackend *);

struct _CCSDynamicBackendInterface
{
    CCSDynamicBackendGetBackendName getBackendName;
    CCSDynamicBackendSupportsRead supportsRead;
    CCSDynamicBackendSupportsWrite supportsWrite;
    CCSDynamicBackendSupportsProfiles supportsProfiles;
    CCSDynamicBackendSupportsIntegration supportsIntegration;
    CCSDynamicBackendGetRawBackend getRawBackend;
};

const char * ccsDynamicBackendGetBackendName (CCSDynamicBackend *);
Bool ccsDynamicBackendSupportsRead (CCSDynamicBackend *);
Bool ccsDynamicBackendSupportsWrite (CCSDynamicBackend *);
Bool ccsDynamicBackendSupportsProfiles (CCSDynamicBackend *);
Bool ccsDynamicBackendSupportsIntegration (CCSDynamicBackend *);
CCSBackend * ccsDynamicBackendGetRawBackend (CCSDynamicBackend *);

unsigned int ccsCCSDynamicBackendInterfaceGetType ();

void ccsFreeDynamicBackend (CCSDynamicBackend *);

/**
 * @brief ccsOpenBackend
 * @param name the name of the backend to open
 * @param interface storage for this backend's interface
 * @return a dlopen handle for this backend
 */
CCSBackend * ccsOpenBackend (const CCSInterfaceTable *, CCSContext *context, const char *name);

/**
 * @brief ccsBackendNewWithDynamicInterface
 * @param context
 * @param interface
 * @param dlhand
 * @return
 *
 * Creates a new CCSBackend for a CCSBackendInterface and dlopen handle
 * dlhand
 */
CCSBackend *
ccsBackendNewWithDynamicInterface (CCSContext *context, const CCSBackendInterface *interface);

CCSBackendInterface* getBackendInfo (void);

COMPIZCONFIG_END_DECLS

#endif
