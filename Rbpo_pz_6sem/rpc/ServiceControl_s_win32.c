

/* this ALWAYS GENERATED file contains the RPC server stubs */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Tue Jan 19 06:14:07 2038
 */
/* Compiler settings for ziOvpo-pz/rpc/ServiceControl.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.01.0628 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if !defined(_M_IA64) && !defined(_M_AMD64) && !defined(_ARM_)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */

#pragma optimize("", off ) 

#include <string.h>
#include "ServiceControl.h"

#define TYPE_FORMAT_STRING_SIZE   19                                
#define PROC_FORMAT_STRING_SIZE   247                               
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


#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need Windows 2000 or later to run this stub because it uses these features:
#error   /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will fail with the RPC_X_WRONG_STUB_VERSION error.
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
/*  8 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x0 ),	/* 0 */
/* 18 */	0x40,		/* Oi2 Flags:  has ext, */
			0x0,		/* 0 */
/* 20 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Procedure RpcGetUserInfo */

/* 28 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 30 */	NdrFcLong( 0x0 ),	/* 0 */
/* 34 */	NdrFcShort( 0x1 ),	/* 1 */
/* 36 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 38 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 40 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 42 */	NdrFcShort( 0x0 ),	/* 0 */
/* 44 */	NdrFcShort( 0x24 ),	/* 36 */
/* 46 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 48 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 50 */	NdrFcShort( 0x0 ),	/* 0 */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */
/* 54 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter isAuthenticated */

/* 56 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 58 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 60 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter username */

/* 62 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 64 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 66 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 68 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 70 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 72 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcLogin */

/* 74 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 76 */	NdrFcLong( 0x0 ),	/* 0 */
/* 80 */	NdrFcShort( 0x2 ),	/* 2 */
/* 82 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 84 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 86 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 88 */	NdrFcShort( 0x0 ),	/* 0 */
/* 90 */	NdrFcShort( 0x8 ),	/* 8 */
/* 92 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 94 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 96 */	NdrFcShort( 0x0 ),	/* 0 */
/* 98 */	NdrFcShort( 0x0 ),	/* 0 */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter username */

/* 102 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 104 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 106 */	NdrFcShort( 0x10 ),	/* Type Offset=16 */

	/* Parameter password */

/* 108 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 110 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 112 */	NdrFcShort( 0x10 ),	/* Type Offset=16 */

	/* Return value */

/* 114 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 116 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 118 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcLogout */

/* 120 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 122 */	NdrFcLong( 0x0 ),	/* 0 */
/* 126 */	NdrFcShort( 0x3 ),	/* 3 */
/* 128 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 130 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 132 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 134 */	NdrFcShort( 0x0 ),	/* 0 */
/* 136 */	NdrFcShort( 0x8 ),	/* 8 */
/* 138 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 140 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 142 */	NdrFcShort( 0x0 ),	/* 0 */
/* 144 */	NdrFcShort( 0x0 ),	/* 0 */
/* 146 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 148 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 150 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 152 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcGetLicenseInfo */

/* 154 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 156 */	NdrFcLong( 0x0 ),	/* 0 */
/* 160 */	NdrFcShort( 0x4 ),	/* 4 */
/* 162 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 164 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 166 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 168 */	NdrFcShort( 0x0 ),	/* 0 */
/* 170 */	NdrFcShort( 0x40 ),	/* 64 */
/* 172 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x4,		/* 4 */
/* 174 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 178 */	NdrFcShort( 0x0 ),	/* 0 */
/* 180 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hasLicense */

/* 182 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 184 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 186 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter blocked */

/* 188 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 190 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 192 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter expirationDate */

/* 194 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 196 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 198 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 200 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 202 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 204 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcActivate */

/* 206 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 208 */	NdrFcLong( 0x0 ),	/* 0 */
/* 212 */	NdrFcShort( 0x5 ),	/* 5 */
/* 214 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 216 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 218 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 220 */	NdrFcShort( 0x0 ),	/* 0 */
/* 222 */	NdrFcShort( 0x8 ),	/* 8 */
/* 224 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 226 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 230 */	NdrFcShort( 0x0 ),	/* 0 */
/* 232 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter activationKey */

/* 234 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 236 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 238 */	NdrFcShort( 0x10 ),	/* Type Offset=16 */

	/* Return value */

/* 240 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 242 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 244 */	0x8,		/* FC_LONG */
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
    28,
    74,
    120,
    154,
    206
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
    0
    };
static const RPC_DISPATCH_TABLE ServiceControl_v1_0_DispatchTable = 
    {
    6,
    (RPC_DISPATCH_FUNCTION*)ServiceControl_table
    };

static const SERVER_ROUTINE ServiceControl_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)RpcRequestStop,
    (SERVER_ROUTINE)RpcGetUserInfo,
    (SERVER_ROUTINE)RpcLogin,
    (SERVER_ROUTINE)RpcLogout,
    (SERVER_ROUTINE)RpcGetLicenseInfo,
    (SERVER_ROUTINE)RpcActivate
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


#endif /* !defined(_M_IA64) && !defined(_M_AMD64) && !defined(_ARM_) */

