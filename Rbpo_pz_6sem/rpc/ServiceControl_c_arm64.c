

/* this ALWAYS GENERATED file contains the RPC client stubs */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Tue Jan 19 06:14:07 2038
 */
/* Compiler settings for ziOvpo-pz/rpc/ServiceControl.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=ARM64 8.01.0628 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if defined(_M_ARM64)


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
#define PROC_FORMAT_STRING_SIZE   281                               
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

#define GENERIC_BINDING_TABLE_SIZE   0            


/* Standard interface: ServiceControl, ver. 1.0,
   GUID={0x5f04e7bb,0x8128,0x4f65,{0x84,0x7c,0x24,0x8e,0xa0,0xf5,0xc7,0x39}} */



static const RPC_CLIENT_INTERFACE ServiceControl___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0x5f04e7bb,0x8128,0x4f65,{0x84,0x7c,0x24,0x8e,0xa0,0xf5,0xc7,0x39}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    0,
    0,
    0,
    0,
    0,
    0x00000000
    };
RPC_IF_HANDLE ServiceControl_v1_0_c_ifspec = (RPC_IF_HANDLE)& ServiceControl___RpcClientInterface;
#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC ServiceControl_StubDesc;
#ifdef __cplusplus
}
#endif

static RPC_BINDING_HANDLE ServiceControl__MIDL_AutoBindHandle;


void RpcRequestStop( 
    /* [in] */ handle_t hBinding)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&ServiceControl_StubDesc,
                  (PFORMAT_STRING) &ServiceControl__MIDL_ProcFormatString.Format[0],
                  hBinding);
    
}


long RpcGetUserInfo( 
    /* [in] */ handle_t hBinding,
    /* [out] */ long *isAuthenticated,
    /* [string][out] */ wchar_t **username)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&ServiceControl_StubDesc,
                  (PFORMAT_STRING) &ServiceControl__MIDL_ProcFormatString.Format[32],
                  hBinding,
                  isAuthenticated,
                  username);
    return ( long  )_RetVal.Simple;
    
}


long RpcLogin( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ const wchar_t *username,
    /* [string][in] */ const wchar_t *password)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&ServiceControl_StubDesc,
                  (PFORMAT_STRING) &ServiceControl__MIDL_ProcFormatString.Format[84],
                  hBinding,
                  username,
                  password);
    return ( long  )_RetVal.Simple;
    
}


long RpcLogout( 
    /* [in] */ handle_t hBinding)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&ServiceControl_StubDesc,
                  (PFORMAT_STRING) &ServiceControl__MIDL_ProcFormatString.Format[136],
                  hBinding);
    return ( long  )_RetVal.Simple;
    
}


long RpcGetLicenseInfo( 
    /* [in] */ handle_t hBinding,
    /* [out] */ long *hasLicense,
    /* [out] */ long *blocked,
    /* [string][out] */ wchar_t **expirationDate)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&ServiceControl_StubDesc,
                  (PFORMAT_STRING) &ServiceControl__MIDL_ProcFormatString.Format[174],
                  hBinding,
                  hasLicense,
                  blocked,
                  expirationDate);
    return ( long  )_RetVal.Simple;
    
}


long RpcActivate( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ const wchar_t *activationKey)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&ServiceControl_StubDesc,
                  (PFORMAT_STRING) &ServiceControl__MIDL_ProcFormatString.Format[234],
                  hBinding,
                  activationKey);
    return ( long  )_RetVal.Simple;
    
}


#if !defined(__RPC_ARM64__)
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
/*  8 */	NdrFcShort( 0x8 ),	/* ARM64 Stack size/offset = 8 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* ARM64 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x0 ),	/* 0 */
/* 18 */	0x40,		/* Oi2 Flags:  has ext, */
			0x0,		/* 0 */
/* 20 */	0xc,		/* 12 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */
/* 28 */	NdrFcShort( 0x1 ),	/* 1 */
/* 30 */	0x1,		/* 1 */
			0x80,		/* 128 */

	/* Procedure RpcGetUserInfo */

/* 32 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 34 */	NdrFcLong( 0x0 ),	/* 0 */
/* 38 */	NdrFcShort( 0x1 ),	/* 1 */
/* 40 */	NdrFcShort( 0x20 ),	/* ARM64 Stack size/offset = 32 */
/* 42 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 44 */	NdrFcShort( 0x0 ),	/* ARM64 Stack size/offset = 0 */
/* 46 */	NdrFcShort( 0x0 ),	/* 0 */
/* 48 */	NdrFcShort( 0x24 ),	/* 36 */
/* 50 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 52 */	0xe,		/* 14 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 54 */	NdrFcShort( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */
/* 60 */	NdrFcShort( 0x3 ),	/* 3 */
/* 62 */	0x3,		/* 3 */
			0x80,		/* 128 */
/* 64 */	0x81,		/* 129 */
			0x82,		/* 130 */

	/* Parameter isAuthenticated */

/* 66 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 68 */	NdrFcShort( 0x8 ),	/* ARM64 Stack size/offset = 8 */
/* 70 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter username */

/* 72 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 74 */	NdrFcShort( 0x10 ),	/* ARM64 Stack size/offset = 16 */
/* 76 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 78 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 80 */	NdrFcShort( 0x18 ),	/* ARM64 Stack size/offset = 24 */
/* 82 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcLogin */

/* 84 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 86 */	NdrFcLong( 0x0 ),	/* 0 */
/* 90 */	NdrFcShort( 0x2 ),	/* 2 */
/* 92 */	NdrFcShort( 0x20 ),	/* ARM64 Stack size/offset = 32 */
/* 94 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 96 */	NdrFcShort( 0x0 ),	/* ARM64 Stack size/offset = 0 */
/* 98 */	NdrFcShort( 0x0 ),	/* 0 */
/* 100 */	NdrFcShort( 0x8 ),	/* 8 */
/* 102 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 104 */	0xe,		/* 14 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 106 */	NdrFcShort( 0x0 ),	/* 0 */
/* 108 */	NdrFcShort( 0x0 ),	/* 0 */
/* 110 */	NdrFcShort( 0x0 ),	/* 0 */
/* 112 */	NdrFcShort( 0x3 ),	/* 3 */
/* 114 */	0x3,		/* 3 */
			0x80,		/* 128 */
/* 116 */	0x81,		/* 129 */
			0x82,		/* 130 */

	/* Parameter username */

/* 118 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 120 */	NdrFcShort( 0x8 ),	/* ARM64 Stack size/offset = 8 */
/* 122 */	NdrFcShort( 0x10 ),	/* Type Offset=16 */

	/* Parameter password */

/* 124 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 126 */	NdrFcShort( 0x10 ),	/* ARM64 Stack size/offset = 16 */
/* 128 */	NdrFcShort( 0x10 ),	/* Type Offset=16 */

	/* Return value */

/* 130 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 132 */	NdrFcShort( 0x18 ),	/* ARM64 Stack size/offset = 24 */
/* 134 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcLogout */

/* 136 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 138 */	NdrFcLong( 0x0 ),	/* 0 */
/* 142 */	NdrFcShort( 0x3 ),	/* 3 */
/* 144 */	NdrFcShort( 0x10 ),	/* ARM64 Stack size/offset = 16 */
/* 146 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 148 */	NdrFcShort( 0x0 ),	/* ARM64 Stack size/offset = 0 */
/* 150 */	NdrFcShort( 0x0 ),	/* 0 */
/* 152 */	NdrFcShort( 0x8 ),	/* 8 */
/* 154 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 156 */	0xc,		/* 12 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 160 */	NdrFcShort( 0x0 ),	/* 0 */
/* 162 */	NdrFcShort( 0x0 ),	/* 0 */
/* 164 */	NdrFcShort( 0x1 ),	/* 1 */
/* 166 */	0x1,		/* 1 */
			0x80,		/* 128 */

	/* Return value */

/* 168 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 170 */	NdrFcShort( 0x8 ),	/* ARM64 Stack size/offset = 8 */
/* 172 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcGetLicenseInfo */

/* 174 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 176 */	NdrFcLong( 0x0 ),	/* 0 */
/* 180 */	NdrFcShort( 0x4 ),	/* 4 */
/* 182 */	NdrFcShort( 0x28 ),	/* ARM64 Stack size/offset = 40 */
/* 184 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 186 */	NdrFcShort( 0x0 ),	/* ARM64 Stack size/offset = 0 */
/* 188 */	NdrFcShort( 0x0 ),	/* 0 */
/* 190 */	NdrFcShort( 0x40 ),	/* 64 */
/* 192 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x4,		/* 4 */
/* 194 */	0x10,		/* 16 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0x4 ),	/* 4 */
/* 204 */	0x4,		/* 4 */
			0x80,		/* 128 */
/* 206 */	0x81,		/* 129 */
			0x82,		/* 130 */
/* 208 */	0x83,		/* 131 */
			0x0,		/* 0 */

	/* Parameter hasLicense */

/* 210 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 212 */	NdrFcShort( 0x8 ),	/* ARM64 Stack size/offset = 8 */
/* 214 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter blocked */

/* 216 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 218 */	NdrFcShort( 0x10 ),	/* ARM64 Stack size/offset = 16 */
/* 220 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter expirationDate */

/* 222 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 224 */	NdrFcShort( 0x18 ),	/* ARM64 Stack size/offset = 24 */
/* 226 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 228 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 230 */	NdrFcShort( 0x20 ),	/* ARM64 Stack size/offset = 32 */
/* 232 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RpcActivate */

/* 234 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 236 */	NdrFcLong( 0x0 ),	/* 0 */
/* 240 */	NdrFcShort( 0x5 ),	/* 5 */
/* 242 */	NdrFcShort( 0x18 ),	/* ARM64 Stack size/offset = 24 */
/* 244 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 246 */	NdrFcShort( 0x0 ),	/* ARM64 Stack size/offset = 0 */
/* 248 */	NdrFcShort( 0x0 ),	/* 0 */
/* 250 */	NdrFcShort( 0x8 ),	/* 8 */
/* 252 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 254 */	0xe,		/* 14 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 256 */	NdrFcShort( 0x0 ),	/* 0 */
/* 258 */	NdrFcShort( 0x0 ),	/* 0 */
/* 260 */	NdrFcShort( 0x0 ),	/* 0 */
/* 262 */	NdrFcShort( 0x2 ),	/* 2 */
/* 264 */	0x2,		/* 2 */
			0x80,		/* 128 */
/* 266 */	0x81,		/* 129 */
			0x0,		/* 0 */

	/* Parameter activationKey */

/* 268 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 270 */	NdrFcShort( 0x8 ),	/* ARM64 Stack size/offset = 8 */
/* 272 */	NdrFcShort( 0x10 ),	/* Type Offset=16 */

	/* Return value */

/* 274 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 276 */	NdrFcShort( 0x10 ),	/* ARM64 Stack size/offset = 16 */
/* 278 */	0x8,		/* FC_LONG */
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
    32,
    84,
    136,
    174,
    234
    };


#ifdef __cplusplus
namespace {
#endif
static const MIDL_STUB_DESC ServiceControl_StubDesc = 
    {
    (void *)& ServiceControl___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &ServiceControl__MIDL_AutoBindHandle,
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
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_ARM64) */

