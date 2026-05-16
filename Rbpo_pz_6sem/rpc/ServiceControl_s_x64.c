

/* this ALWAYS GENERATED file contains the RPC server stubs */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Tue Jan 19 06:14:07 2038
 */
/* Compiler settings for ziOvpo-pz/rpc/ServiceControl.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0628 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/

#include <string.h>
#include "ServiceControl.h"

#define TYPE_FORMAT_STRING_SIZE   19                                
#define PROC_FORMAT_STRING_SIZE   259                               
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _ServiceControl_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } ServiceControl_MIDL_TYPE_FORMAT_STRING;

typedef struct _ServiceControl_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } ServiceControl_MIDL_PROC_FORMAT_STRING;

typedef struct _ServiceControl_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } ServiceControl_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax_2_0 = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};

#if defined(_CONTROL_FLOW_GUARD_XFG)
#define XFG_TRAMPOLINES(ObjectType)\
NDR_SHAREABLE unsigned long ObjectType ## _UserSize_XFG(unsigned long * pFlags, unsigned long Offset, void * pObject)\
{\
return  ObjectType ## _UserSize(pFlags, Offset, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserMarshal_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserMarshal(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserUnmarshal_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserUnmarshal(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE void ObjectType ## _UserFree_XFG(unsigned long * pFlags, void * pObject)\
{\
ObjectType ## _UserFree(pFlags, (ObjectType *)pObject);\
}
#define XFG_TRAMPOLINES64(ObjectType)\
NDR_SHAREABLE unsigned long ObjectType ## _UserSize64_XFG(unsigned long * pFlags, unsigned long Offset, void * pObject)\
{\
return  ObjectType ## _UserSize64(pFlags, Offset, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserMarshal64_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserMarshal64(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserUnmarshal64_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserUnmarshal64(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE void ObjectType ## _UserFree64_XFG(unsigned long * pFlags, void * pObject)\
{\
ObjectType ## _UserFree64(pFlags, (ObjectType *)pObject);\
}
#define XFG_BIND_TRAMPOLINES(HandleType, ObjectType)\
static void* ObjectType ## _bind_XFG(HandleType pObject)\
{\
return ObjectType ## _bind((ObjectType) pObject);\
}\
static void ObjectType ## _unbind_XFG(HandleType pObject, handle_t ServerHandle)\
{\
ObjectType ## _unbind((ObjectType) pObject, ServerHandle);\
}
#define XFG_TRAMPOLINE_FPTR(Function) Function ## _XFG
#define XFG_TRAMPOLINE_FPTR_DEPENDENT_SYMBOL(Symbol) Symbol ## _XFG
#else
#define XFG_TRAMPOLINES(ObjectType)
#define XFG_TRAMPOLINES64(ObjectType)
#define XFG_BIND_TRAMPOLINES(HandleType, ObjectType)
#define XFG_TRAMPOLINE_FPTR(Function) Function
#define XFG_TRAMPOLINE_FPTR_DEPENDENT_SYMBOL(Symbol) Symbol
#endif

extern const ServiceControl_MIDL_TYPE_FORMAT_STRING ServiceControl__MIDL_TypeFormatString;
extern const ServiceControl_MIDL_PROC_FORMAT_STRING ServiceControl__MIDL_ProcFormatString;
extern const ServiceControl_MIDL_EXPR_FORMAT_STRING ServiceControl__MIDL_ExprFormatString;

/* Standard interface: ServiceControl, ver. 1.0,
   GUID={0x5f04e7bb,0x8128,0x4f65,{0x84,0x7c,0x24,0x8e,0xa0,0xf5,0xc7,0x39}} */


extern const MIDL_SERVER_INFO ServiceControl_ServerInfo;

extern const RPC_DISPATCH_TABLE ServiceControl_v1_0_DispatchTable;

static const RPC_SERVER_INTERFACE ServiceControl___RpcServerInterface =
    {
    sizeof(RPC_SERVER_INTERFACE),
    {{0x5f04e7bb,0x8128,0x4f65,{0x84,0x7c,0x24,0x8e,0xa0,0xf5,0xc7,0x39}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    (RPC_DISPATCH_TABLE*)&ServiceControl_v1_0_DispatchTable,
    0,
    0,
    0,
    &ServiceControl_ServerInfo,
    0x04000000
    };
RPC_IF_HANDLE ServiceControl_v1_0_s_ifspec = (RPC_IF_HANDLE)& ServiceControl___RpcServerInterface;
#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC ServiceControl_StubDesc;
#ifdef __cplusplus
}
#endif


#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const ServiceControl_MIDL_PROC_FORMAT_STRING ServiceControl__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure RpcRequestStop */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x0 ),	/* 0 */
/* 18 */	0x40,		/* Oi2 Flags:  has ext, */
			0x0,		/* 0 */
/* 20 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Procedure RpcGetUserInfo */

/* 30 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 32 */	NdrFcLong( 0x0 ),	/* 0 */
/* 36 */	NdrFcShort( 0x1 ),	/* 1 */
/* 38 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 40 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 42 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 44 */	NdrFcShort( 0x0 ),	/* 0 */
/* 46 */	NdrFcShort( 0x24 ),	/* 36 */
/* 48 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 50 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */
/* 54 */	NdrFcShort( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter isAuthenticated */

/* 60 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 62 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 64 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter username */

/* 66 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 68 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 70 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 72 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 74 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 76 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcLogin */

/* 78 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 80 */	NdrFcLong( 0x0 ),	/* 0 */
/* 84 */	NdrFcShort( 0x2 ),	/* 2 */
/* 86 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 88 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 90 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 92 */	NdrFcShort( 0x0 ),	/* 0 */
/* 94 */	NdrFcShort( 0x8 ),	/* 8 */
/* 96 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 98 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */
/* 102 */	NdrFcShort( 0x0 ),	/* 0 */
/* 104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 106 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter username */

/* 108 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 110 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 112 */	NdrFcShort( 0x10 ),	/* Type Offset=16 */

	/* Parameter password */

/* 114 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 116 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 118 */	NdrFcShort( 0x10 ),	/* Type Offset=16 */

	/* Return value */

/* 120 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 122 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 124 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcLogout */

/* 126 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 128 */	NdrFcLong( 0x0 ),	/* 0 */
/* 132 */	NdrFcShort( 0x3 ),	/* 3 */
/* 134 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 136 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 138 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 140 */	NdrFcShort( 0x0 ),	/* 0 */
/* 142 */	NdrFcShort( 0x8 ),	/* 8 */
/* 144 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 146 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 148 */	NdrFcShort( 0x0 ),	/* 0 */
/* 150 */	NdrFcShort( 0x0 ),	/* 0 */
/* 152 */	NdrFcShort( 0x0 ),	/* 0 */
/* 154 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 156 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 158 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 160 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcGetLicenseInfo */

/* 162 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 164 */	NdrFcLong( 0x0 ),	/* 0 */
/* 168 */	NdrFcShort( 0x4 ),	/* 4 */
/* 170 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 172 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 174 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 178 */	NdrFcShort( 0x40 ),	/* 64 */
/* 180 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x4,		/* 4 */
/* 182 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 184 */	NdrFcShort( 0x0 ),	/* 0 */
/* 186 */	NdrFcShort( 0x0 ),	/* 0 */
/* 188 */	NdrFcShort( 0x0 ),	/* 0 */
/* 190 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hasLicense */

/* 192 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 194 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 196 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter blocked */

/* 198 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 200 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 202 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter expirationDate */

/* 204 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 206 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 208 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 210 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 212 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 214 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcActivate */

/* 216 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 218 */	NdrFcLong( 0x0 ),	/* 0 */
/* 222 */	NdrFcShort( 0x5 ),	/* 5 */
/* 224 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 226 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 228 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 230 */	NdrFcShort( 0x0 ),	/* 0 */
/* 232 */	NdrFcShort( 0x8 ),	/* 8 */
/* 234 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 236 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 238 */	NdrFcShort( 0x0 ),	/* 0 */
/* 240 */	NdrFcShort( 0x0 ),	/* 0 */
/* 242 */	NdrFcShort( 0x0 ),	/* 0 */
/* 244 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter activationKey */

/* 246 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 248 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 250 */	NdrFcShort( 0x10 ),	/* Type Offset=16 */

	/* Return value */

/* 252 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 254 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 256 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcGetAvDatabaseInfo */

/* 258 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 260 */	NdrFcLong( 0x0 ),	/* 0 */
/* 264 */	NdrFcShort( 0x6 ),	/* 6 */
/* 266 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 268 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 270 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 272 */	NdrFcShort( 0x0 ),	/* 0 */
/* 274 */	NdrFcShort( 0x40 ),	/* 64 */
/* 276 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x4,		/* 4 */
/* 278 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 280 */	NdrFcShort( 0x0 ),	/* 0 */
/* 282 */	NdrFcShort( 0x0 ),	/* 0 */
/* 284 */	NdrFcShort( 0x0 ),	/* 0 */
/* 286 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter isLoaded */

/* 288 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 290 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 292 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter releaseDate */

/* 294 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 296 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 298 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter recordCount */

/* 300 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 302 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 304 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 306 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 308 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 310 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcScanPath */

/* 312 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 314 */	NdrFcLong( 0x0 ),	/* 0 */
/* 318 */	NdrFcShort( 0x7 ),	/* 7 */
/* 320 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 322 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 324 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 326 */	NdrFcShort( 0x0 ),	/* 0 */
/* 328 */	NdrFcShort( 0x38 ),	/* 56 */
/* 330 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 332 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */
/* 336 */	NdrFcShort( 0x0 ),	/* 0 */
/* 338 */	NdrFcShort( 0x0 ),	/* 0 */
/* 340 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter path */

/* 342 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 344 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 346 */	NdrFcShort( 0x10 ),	/* Type Offset=16 */

	/* Parameter isFolder */

/* 348 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 350 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 352 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter infected */

/* 354 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 356 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 358 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter summary */

/* 360 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 362 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 364 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 366 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 368 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 370 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const ServiceControl_MIDL_TYPE_FORMAT_STRING ServiceControl__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/*  4 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/*  8 */	NdrFcShort( 0x2 ),	/* Offset= 2 (10) */
/* 10 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 12 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 14 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 16 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

static const unsigned short ServiceControl_FormatStringOffsetTable[] =
    {
    0,
    30,
    78,
    126,
    162,
    216,
    258,
    312
    };


#ifdef __cplusplus
namespace {
#endif
static const MIDL_STUB_DESC ServiceControl_StubDesc = 
    {
    (void *)& ServiceControl___RpcServerInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    0,
    0,
    0,
    0,
    0,
    ServiceControl__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x8010274, /* MIDL Version 8.1.628 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };
#ifdef __cplusplus
}
#endif

static const RPC_DISPATCH_FUNCTION ServiceControl_table[] =
    {
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    0
    };
static const RPC_DISPATCH_TABLE ServiceControl_v1_0_DispatchTable = 
    {
    8,
    (RPC_DISPATCH_FUNCTION*)ServiceControl_table
    };

static const SERVER_ROUTINE ServiceControl_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)RpcRequestStop,
    (SERVER_ROUTINE)RpcGetUserInfo,
    (SERVER_ROUTINE)RpcLogin,
    (SERVER_ROUTINE)RpcLogout,
    (SERVER_ROUTINE)RpcGetLicenseInfo,
    (SERVER_ROUTINE)RpcActivate,
    (SERVER_ROUTINE)RpcGetAvDatabaseInfo,
    (SERVER_ROUTINE)RpcScanPath
    };

static const MIDL_SERVER_INFO ServiceControl_ServerInfo = 
    {
    &ServiceControl_StubDesc,
    ServiceControl_ServerRoutineTable,
    ServiceControl__MIDL_ProcFormatString.Format,
    ServiceControl_FormatStringOffsetTable,
    0,
    0,
    0,
    0};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/

