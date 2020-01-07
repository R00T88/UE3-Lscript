/******************************************************************************
*
* LScript Introduction:
* LScript is a tiny scripting engine for the 2.6 version of the AAO game.  
* LScript connects a Lua scripting engine up to the 2.6 game engine.  You 
* can learn more about the Lua scripting language here:
* http://www.lua.org
*
* There is a LScript.txt file that describes how to use this code from Lua.
*
* How to build LScript:
*   1. Download the Lua 5.1 source code from the Lua site.
*   2. Copy all the files from lua-5.1/src directory into your build directory.
*   3. Delete the lua.c and luac.c files.
*   4. Copy this file into the build directory.
*   5. Create a project that includes all the files in the build directory.
*   6. Build it.ylow

*
* Picklelicious	1  Apr 2006	Original Version - April fools day
* Temp2			5  Nov 2006 Converted for AA 2.7.0 - Fireworks night
* Temp2			6  Nov 2006 Ported to UT3369 - Melbourne Cup
* SmokeZ		13 Nov 2006 Added Some Extra Features + Fixed Common Compiler Errors
* Temp2			13 Nov 2006 Changed some stuff for VS 2005
* Temp2			15 Nov 2006 New hooks
* Temp2			24 Dec 2006 Converted for AA 2.8.0 - Christmas Eve
* Temp2			4  Apr 2007 Converted for AA 2.8.1 - Easter Parade.
* HUMM3R		5  Oct 2008 Converted for GOW on UE3
* R00T88		7  Jan 2010 Converted for UT3 on UE3
******************************************************************************/

/******************************************************************************
*	you need to set the project configuration to RELEASE
*	or the dll will not work!
*	for more infomations about that look here:
*
*	http://www.mpcforum.com/showpost.php?p=1542459&postcount=62
*
*
*	changelog:
*
*	(13.11 - smokez)
*	{
*		key-defines for (restart), (open logfile), (open lua console)
*		key-combos only work if the game is also the foregroundwindow
*		if you restart the lua-engine the logfile will restart also
*		the "'CacheData' : redefinition" error shuld be fixed
*		removed the ddraw header coz its not needed
*		you can define the default log/botfile now
*		if the logfile-define is empty "" the log feature will NOT be used
*		bot/logfile now use the lib path as base and not the executable path
*	}
*
*	FIXTHIS: (line 3152) the ut-window-text is missing
*
* IMPORTANT : Use 4-byte alignment (or aligment used by the engine) !!!
*
******************************************************************************/

#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1
//#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

/* 
	USER CONFIG
*/
	
#define DEFAULT_BOTFILE		"default.lua"
#define DEFAULT_LOGFILE		"default.log"

#define BASEKEY				VK_MENU		//,VK_SHIFT
#define RESTART_KEY			'r'			//BASEKEY + <your_key>
#define OPEN_LOG_KEY		'l'			//BASEKEY + <your_key>
#define OPEN_LUA_CONSOLE	'c'			//BASEKEY + <your_key>

/*
	USER CONFIG END
*/

#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <time.h>
#include <stdlib.h>

#include "detours.h"

extern "C" 
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#pragma comment(lib, "Toolkit.lib")
#include "Toolkit.h"

#include "vmthooks.cpp"

#define EngineUFunction "Engine.UFunction"
#define EngineUObject   "Engine.UObject"
#define EngineUProperty "Engine.UProperty"
#define MaxFullNameSize 512
#define MaxNameSize     128
#define NoIndex         0x7FFFFFFF

void *processEvent = (void*)0xBD3F50;

/******************************************************************************
*
* LogFile
*
******************************************************************************/
FILE* hLogFile;
char szLogFile[MAX_PATH];
bool bLogActive = false;

void Log(char * pFormat, ...)
{
	va_list     ArgList;
	char        Line[32];
	time_t      Time;
	struct tm * TM;

	if(hLogFile && bLogActive)
	{
		Time = time(0);
		TM = localtime(&Time);
		strftime(Line, sizeof(Line), "%H:%M:%S ", TM);
		fwrite(Line, strlen(Line), 1, hLogFile);

		va_start(ArgList, pFormat);
		vfprintf(hLogFile, pFormat, ArgList);
		va_end(ArgList);

		fprintf(hLogFile, "\n");
		fflush(hLogFile);
	}
}


/******************************************************************************
*
* Game Engine (Game dependent stuff here too)
*
******************************************************************************/
#define WINDOWTEXT "Unreal Tournament 3"
#define LEVEL_NAME "World UTFrontEnd.TheWorld"
#define VIEWPORT_NAME "LocalPlayer Transient.GameEngine.LocalPlayer"
#define HASH_ADDRESS 0x1DDDF00
#define OBJECTS_ADDRESS 0x1DE5FA4
#define NAMES_ADDRESS 0x1DD46E8

template<class Type> struct TArray
{
	Type * pArray;
	DWORD  Count;
	DWORD  Max;
};

typedef DWORD FName;


struct FNameEntry
{
	int   FNameIndex;
	DWORD Unknown0x004;
	DWORD Unknown0x008;
  	DWORD Unknown0x00C;
	WCHAR Name[1];
};


struct UClass;
struct UObject
{
	DWORD **     pVMT;
	DWORD        ObjectInternal;
	DWORD        ObjectFlags1;
	DWORD        ObjectFlags2;
	UObject *    pHash;
  	UObject *    pHashOuter;
	DWORD *      pFrame;
	DWORD *      pLinkerLoad;
	UObject *    pLinkerIndex;
	DWORD        NetIndex;
	UObject *    pOuter;
	FName        Name;
	DWORD        Dummy0;
	UClass *     pClass;
  	UObject *    pArchetype; // 0x38


	void GetCPPName(char * pBuffer);
	void GetFullName(char * pBuffer);
	void GetName(char * pBuffer);
	int  IsA(UClass * pClass);
	/*
	static class UClass * CDECL StaticLoadClass(class UClass *, class UObject *, TCHAR const *, TCHAR const *, DWORD, class UPackageMap *);
	static class UObject * CDECL StaticLoadObject(class UClass *, class UObject *, TCHAR const *, TCHAR const *, DWORD, class UPackageMap *);
	*/
};


struct UField : UObject 
{
	UField *           pSuper;	//0x3C
	UField *           pNext;	//0x40
};

struct ULevelBase : UObject
{
	TArray<UObject *>   ActorArray;
	unsigned char                                    UnknownData0003[ 0x48 ];
};

struct ULevel : ULevelBase
{
	unsigned char                                      UnknownData0164[ 0x100 ]; 
};

struct UWorld : UObject
{
	unsigned char                                      UnknownData0004[ 0x8 ];
	TArray< ULevel* >                            Levels;                                           
	unsigned char                                      UnknownData0005[ 0x200 ];
};

struct AHUD
{
  char Unknown0x408[0x408];
  UObject* Canvas;
};

struct UProperty : UField 
{
	DWORD              ElementCount;	// 0x44
	DWORD              ElementSize;		// 0x48
	DWORD              Flags;			// 0x4C
  	FName              Category;		//0x50
  	DWORD              Dummy1;			//0x54
	DWORD              ReplicationOffset;	//0x58
	DWORD              ReplicationIndex;	//0x5C
	DWORD              Unknown3;			//0x60
	DWORD              CStructOffset;		//0x64
	char               Unknown0x005C[0x1C];
	UClass *           pRelatedClass;
};

struct UStruct : UField 
{
	DWORD              ScriptText;	//0x44
	DWORD              CppText;		//0x48
	UField *           pChildren;	//0x4C
	DWORD              Size;		//0x50
	TArray<BYTE>       Script;		//0x54
	char               Unknown0x0060[0x030];
};


struct UFunction : UStruct 
{
	DWORD              Flags;
	WORD               NativeIndex;
	WORD                    RepOffset;
	unsigned char           OperPrecedence;	
	FName				FriendlyName;							
	unsigned char           NumParms;					
	WORD                    ParmsSize;					
	WORD                    ReturnValueOffset;			
	void*                   Func;	
};


struct UState : UStruct 
{
};


struct UClass : UState 
{
};


struct AActor : UObject
{
  /*
	char               Unknown0x0034[0x110];
	ULevel *           XLevel;			   //0x0134 013c
  */
};

//Viewport not used , just declared for compatibility
struct UViewport : UObject
{
	char               Unknown0x003C[0x004]; // 0x3C
	AActor *           Actor;
};

TArray<UObject *> *    pAActorArray;
TArray<FNameEntry *> * pFNameEntryArray;
TArray<UObject *> *    pUObjectArray;
UObject **             pUObjectHash;

UViewport *            pViewport;

UClass *               pActorClass;
UClass *               pClassClass;
UClass *               pFunctionClass;
UClass *               pStructClass;

UClass *               pArrayPropertyClass;
UClass *               pBoolPropertyClass;
UClass *               pBytePropertyClass;
UClass *               pClassPropertyClass;
UClass *               pDelegatePropertyClass;
UClass *               pFixedArrayPropertyClass;
UClass *               pFloatPropertyClass;
UClass *               pIntPropertyClass;
UClass *               pMapPropertyClass;
UClass *               pNamePropertyClass;
UClass *               pObjectPropertyClass;
UClass *               pPointerPropertyClass;
UClass *               pPropertyClass;
UClass *               pStructPropertyClass;
UClass *               pStrPropertyClass;
UClass *               pInterfacePropertyClass;
UClass *               pComponentPropertyClass;

//void (__stdcall *      pFString_ConstructPChar)(char *);
//void (__stdcall *      pFString_Destruct)(void);


UObject * FindActorByFullName(const char * pFullName) 
{
	char       Name[MaxFullNameSize];
	int        ObjectCount;
	UObject *  pObject;
	UObject ** ppObject;

	UWorld* pWorld = *(UWorld **)0x1DEE5EC;
	pAActorArray = &pWorld->Levels.pArray[0]->ActorArray;

	ObjectCount = pAActorArray->Count;
	ppObject = (UObject **)pAActorArray->pArray;
	while (ObjectCount)
	{
		pObject = *ppObject;
		if (pObject)
		{
			pObject->GetFullName(Name);
			if (!strcmp(Name, pFullName))
				return(pObject);
		}

		++ppObject;
		--ObjectCount;
	}

	return(0);
}


UObject * FindObjectByFullName(const char * pFullName) 
{
	char       Name[MaxFullNameSize];
	int        ObjectCount;
	UObject *  pObject;
	UObject ** ppObject;

	ObjectCount = pUObjectArray->Count;
	ppObject = (UObject **)pUObjectArray->pArray;
	while (ObjectCount)
	{
		pObject = *ppObject;
		if (pObject)
		{
			pObject->GetFullName(Name);
			if (!strcmp(Name, pFullName))
				return(pObject);
		}

		++ppObject;
		--ObjectCount;
	}

	return(0);
}

UObject * FindObjectByName(const char * pName) 
{
	char       Name[MaxFullNameSize];
	int        ObjectCount;
	UObject *  pObject;
	UObject ** ppObject;


	ObjectCount = pUObjectArray->Count;
	ppObject = (UObject **)pUObjectArray->pArray;
	while (ObjectCount)
	{
		pObject = *ppObject;
		if (pObject)
		{
			pObject->GetName(Name);
			if (!strcmp(Name, pName))
				return(pObject);
		}

		++ppObject;
		--ObjectCount;
	}

	return(0);
}

WCHAR * GetFName(FName Name)
{
	if ((Name >= pFNameEntryArray->Count) || !pFNameEntryArray->pArray[Name])
		return(L"[empty]");

	return(pFNameEntryArray->pArray[Name]->Name);
}


void UObject::GetCPPName(char * pBuffer)
{
	if (pClass == pStructClass)
	{
		*pBuffer = 'F';
	}
	else
	{
		UClass * pNextClass;

		*pBuffer = 'U';

		if (pClass == pClassClass)
		        pNextClass = (UClass *)this;
		else
			pNextClass = this->pClass;
		while (pNextClass)
		{
			if (pNextClass == pActorClass)
			{
				*pBuffer = 'A';
				break;
			}

			if (pNextClass == pNextClass->pSuper)
				break;

			pNextClass = (UClass *)pNextClass->pSuper;
		}
	}

	sprintf(&pBuffer[1], "%S", GetFName(Name));
}


void AddPath(UObject * pObject, char * pBuffer)
{
	if (pObject->pOuter)
		AddPath(pObject->pOuter, pBuffer);

	sprintf(pBuffer, "%S.", GetFName(pObject->Name));
}


void UObject::GetFullName(char * pBuffer)
{
	/*if(this->pClass)
		sprintf(pBuffer, "%S ", GetFName(this->pClass->Name));
	else
		sprintf(pBuffer, "%S ", GetFName(this->Name));

	if (this->pOuter)
		AddPath(this->pOuter, &pBuffer[strlen(pBuffer)]);

	sprintf(&pBuffer[strlen(pBuffer)], "%S", GetFName(this->Name));*/

	if(this->pClass && this->pOuter) 
	{ 
		if(this->pOuter->pOuter) 
			sprintf_s(&pBuffer[0], 256, "%S %S.%S.%S", GetFName(this->pClass->Name), GetFName(this->pOuter->pOuter->Name), GetFName(this->pOuter->Name), GetFName(this->Name)); 
		else 
			sprintf_s(&pBuffer[0], 256, "%S %S.%S", GetFName(this->pClass->Name), GetFName(this->pOuter->Name), GetFName(this->Name)); 
	} 
	else 
		sprintf_s(&pBuffer[0], 256, "(null)"); 

}


void UObject::GetName(char * pBuffer)
{
	sprintf(pBuffer, "%S", GetFName(Name));
}


int UObject::IsA(UClass * pClass)
{
	UClass * pNextClass;

	if(!this->pClass)
		return(0);

	pNextClass = this->pClass;


	while (pNextClass)
	{
		if (pNextClass == pClass)
			return(1);


		if (!pNextClass->pSuper || pNextClass == pNextClass->pSuper)
			return(0);


		if(!pNextClass->pSuper)
			return 0;
		else
			pNextClass = (UClass *)pNextClass->pSuper;
	}

	return(0);
}


void AppendType(char * pString, UProperty * pProperty)
{
	if (pProperty->IsA(pArrayPropertyClass))
	{
		strcat(pString, "TArray<");
		AppendType(pString, (UProperty *)pProperty->pRelatedClass);
		strcat(pString, ">");
	}

	else if (pProperty->IsA(pBoolPropertyClass))
	{
		strcat(pString, "bool");
	}

	else if (pProperty->IsA(pBytePropertyClass))
	{
		strcat(pString, "byte");
	}

	else if (pProperty->IsA(pClassPropertyClass))
	{
		strcat(pString, "UClass *");
	}

	else if (pProperty->IsA(pDelegatePropertyClass))
	{
		strcat(pString, "delegate");
	}

	else if (pProperty->IsA(pFixedArrayPropertyClass))
	{
		strcat(pString, "fixed");
	}

	else if (pProperty->IsA(pFloatPropertyClass))
	{
		strcat(pString, "float");
	}

	else if (pProperty->IsA(pIntPropertyClass))
	{
		strcat(pString, "int");
	}

	else if (pProperty->IsA(pMapPropertyClass))
	{
		strcat(pString, "map");
	}

	else if (pProperty->IsA(pNamePropertyClass))
	{
		strcat(pString, "FName");
	}

	else if (pProperty->IsA(pObjectPropertyClass))
	{
		pProperty->pRelatedClass->GetCPPName(&pString[strlen(pString)]);
		strcat(pString, " *");
	}

	else if (pProperty->IsA(pPointerPropertyClass))
	{
		strcat(pString, "void *");
	}

	else if (pProperty->IsA(pStrPropertyClass))
	{
		strcat(pString, "FString");
	}

	else if (pProperty->IsA(pStructPropertyClass))
	{
		pProperty->pRelatedClass->GetCPPName(&pString[strlen(pString)]);
	}

	else if (pProperty->IsA(pInterfacePropertyClass))
	{
		pProperty->pRelatedClass->GetCPPName(&pString[strlen(pString)]);
	}

	else if (pProperty->IsA(pComponentPropertyClass))
	{
		strcat(pString, "component");
	}

	else
	{
		strcat(pString, "[unknown]");
	}
}


void AppendValue(char * pString, UProperty * pProperty, char * pData, int Index)
{
	if (pProperty->IsA(pArrayPropertyClass))
	{
		TArray<void> * pValue;

		pValue = (TArray<void> *)(pData + pProperty->CStructOffset);
		if (!pValue->Count || !pValue->pArray)
		{
			sprintf(&pString[strlen(pString)], "[%d][%d][nil]", pValue->Count, pValue->Max);
		}
		else
		{
			sprintf(&pString[strlen(pString)], "[%d][%d]", pValue->Count, pValue->Max);
			AppendValue(pString, (UProperty *)pProperty->pRelatedClass, (char *)pValue->pArray, 0);
		}
	}

	else if (pProperty->IsA(pBoolPropertyClass))
	{
		int Value;

		Value = *(int *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		Value &= (DWORD)pProperty->pRelatedClass;
		if (Value)
			Value = 1;
		sprintf(&pString[strlen(pString)], "[%d]", Value);
	}

	else if (pProperty->IsA(pBytePropertyClass))
	{
		int Value;

		Value = *(BYTE *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		sprintf(&pString[strlen(pString)], "[%d]", Value);
	}

	else if (pProperty->IsA(pClassPropertyClass))
	{
		int Value;

		Value = *(int *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		sprintf(&pString[strlen(pString)], "[0x00%X]", Value);
	}

	else if (pProperty->IsA(pFloatPropertyClass))
	{
		float Value;

		Value = *(float *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		sprintf(&pString[strlen(pString)], "[%0.3f]", Value);
	}

	else if (pProperty->IsA(pIntPropertyClass))
	{
		int Value;

		Value = *(int *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		sprintf(&pString[strlen(pString)], "[%d]", Value);
	}

	else if (pProperty->IsA(pNamePropertyClass))
	{
		FName Value;

		Value = *(FName *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		sprintf(&pString[strlen(pString)], "[%S]", GetFName(Value));
	}

	else if (pProperty->IsA(pObjectPropertyClass))
	{
		UObject * pValue;

		pValue = *(UObject **)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		if (pValue)
			sprintf(&pString[strlen(pString)], "[%S]", GetFName(pValue->Name));
		else
			strcat(pString, "[nil]");
	}

	else if (pProperty->IsA(pPointerPropertyClass))
	{
		int Value;

		Value = *(int *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		sprintf(&pString[strlen(pString)], "[0x00%X]", Value);
	}

	else if (pProperty->IsA(pStrPropertyClass))
	{
		TArray<WCHAR> * Value;

		Value = (TArray<WCHAR> *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		if (Value->pArray)
			sprintf(&pString[strlen(pString)], "[%S]", Value->pArray);
		else
			strcat(pString, "[nil]");
	}

	else if (pProperty->IsA(pStructPropertyClass))
	{
		UProperty * pSubProp;

		pSubProp = (UProperty *)pProperty->pRelatedClass->pChildren;
		while (pSubProp)
		{
			if (pSubProp->IsA(pPropertyClass))
			{
				pSubProp->GetName(&pString[strlen(pString)]);
				AppendValue(pString, pSubProp, pData + pProperty->CStructOffset +
					(pProperty->ElementSize * Index), 0);
				strcat(pString, " ");
			}

			pSubProp = (UProperty *)pSubProp->pNext;
		}
	}

	else if (pProperty->IsA(pDelegatePropertyClass))
	{
		UObject * pValue;

		pValue = *(UObject **)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		if (pValue)
			sprintf(&pString[strlen(pString)], "[%S]", GetFName(pValue->Name));
		else
			strcat(pString, "[nil]");
	}

	else if (pProperty->IsA(pInterfacePropertyClass))
	{
		UObject * pValue;

		pValue = *(UObject **)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		if (pValue)
			sprintf(&pString[strlen(pString)], "[%S]", GetFName(pValue->Name));
		else
			strcat(pString, "[nil]");
	}
	//pFixedArrayPropertyClass
	//pMapPropertyClass
  //pComponentClass

}

wchar_t* convertToWide(const char* orig) {
  size_t origsize = strlen(orig) + 1;
  const size_t newsize = 1024;
  size_t convertedChars = 0;
  wchar_t wcstring[newsize];
  convertedChars = mbstowcs(wcstring, orig, origsize);
  return _wcsdup(wcstring);
}

void FStringConstructor(void * pThis, const char * pString)
{
  ((TArray<CHAR>*)pThis)->Count = (*pString ? strlen(pString) * sizeof(wchar_t) + 1 : 0);
  ((TArray<CHAR>*)pThis)->Max = ((TArray<CHAR>*)pThis)->Count;
  ((TArray<CHAR>*)pThis)->pArray = (char*)malloc(((TArray<CHAR>*)pThis)->Count * sizeof(wchar_t));
  wchar_t* wc = convertToWide(pString);
  if(((TArray<CHAR>*)pThis)->Count > 0) {
				memcpy(&((TArray<CHAR>*)pThis)->pArray[0], wc, ((TArray<CHAR>*)pThis)->Count*sizeof(wchar_t) );
  }
  free(wc);
  /*
	__asm mov  eax,pString;
	__asm push eax
		__asm mov  ecx,pThis;
	__asm call pFString_ConstructPChar;
  */
}

void FStringDestructor(void * pThis) 
{
  try {
    ((TArray<CHAR>*)pThis)->Count = 0;
    ((TArray<CHAR>*)pThis)->Max = 0;
    if (((TArray<CHAR>*)pThis)->pArray) {
      free(((TArray<CHAR>*)pThis)->pArray);
    }
  } catch (...) {
    Log("Error in FStringDestructor, it was internally created");
  }
/*
	__asm mov  ecx,pThis;
	__asm call pFString_Destruct;
  */
}

void InitNameArray() {
    pFNameEntryArray = (TArray<FNameEntry *> *) NAMES_ADDRESS;
}

void EngineInit(void)
{
	if (!pUObjectArray)
	{
		//HMODULE hCore;
		//hCore = GetModuleHandle("core.dll");
		//Log("1");

		InitNameArray();

		//Log("2");
      //(TArray<FNameEntry *> *)GetProcAddress(hCore, "?Names@FName@@0V?$TArray@PAUFNameEntry@@@@A");
		//pFString_ConstructPChar = (void (__stdcall *)(char *))GetProcAddress(hCore, "??0FString@@QAE@PBD@Z");
		//pFString_Destruct = (void (__stdcall *)(void))GetProcAddress(hCore, "??1FString@@QAE@XZ");
    pUObjectArray = (TArray<UObject *> *) OBJECTS_ADDRESS;
      //(TArray<UObject *> *)GetProcAddress(hCore, "?GObjObjects@UObject@@0V?$TArray@PAVUObject@@@@A");
		pUObjectHash = (UObject **) HASH_ADDRESS;
      //(UObject **)GetProcAddress(hCore, "?GObjHash@UObject@@0PAPAV1@A");
    pViewport = (UViewport *)FindObjectByFullName(VIEWPORT_NAME);

	//Log("3");

		pActorClass    = (UClass *)FindObjectByFullName("Class Engine.Actor");
		pClassClass    = (UClass *)FindObjectByFullName("Class Core.Class");
		pFunctionClass = (UClass *)FindObjectByFullName("Class Core.Function");
		pStructClass   = (UClass *)FindObjectByFullName("Class Core.ScriptStruct");

		pArrayPropertyClass      = (UClass *)FindObjectByFullName("Class Core.ArrayProperty");
		pBoolPropertyClass       = (UClass *)FindObjectByFullName("Class Core.BoolProperty");
		pBytePropertyClass       = (UClass *)FindObjectByFullName("Class Core.ByteProperty");
		pClassPropertyClass      = (UClass *)FindObjectByFullName("Class Core.ClassProperty");
		pDelegatePropertyClass   = (UClass *)FindObjectByFullName("Class Core.DelegateProperty");
		pFixedArrayPropertyClass = (UClass *)FindObjectByFullName("Class Core.FixedArrayProperty");
		pFloatPropertyClass      = (UClass *)FindObjectByFullName("Class Core.FloatProperty");
		pIntPropertyClass        = (UClass *)FindObjectByFullName("Class Core.IntProperty");
		pMapPropertyClass        = (UClass *)FindObjectByFullName("Class Core.MapProperty");
		pNamePropertyClass       = (UClass *)FindObjectByFullName("Class Core.NameProperty");
		pObjectPropertyClass     = (UClass *)FindObjectByFullName("Class Core.ObjectProperty");
		pPointerPropertyClass    = (UClass *)FindObjectByFullName("Class Core.PointerProperty");
		pPropertyClass           = (UClass *)FindObjectByFullName("Class Core.Property");
		pStructPropertyClass     = (UClass *)FindObjectByFullName("Class Core.StructProperty");
		pStrPropertyClass        = (UClass *)FindObjectByFullName("Class Core.StrProperty");
		pInterfacePropertyClass  = (UClass *)FindObjectByFullName("Class Core.InterfaceProperty");
		pComponentPropertyClass  = (UClass *)FindObjectByFullName("Class Core.ComponentProperty");

		//Log("4");
	}
}


/******************************************************************************
*
* Lua Engine Functions
*
******************************************************************************/
struct UFunctionData
{
	UObject *   pObject;
	UFunction * pFunction;
};

struct UObjectData
{
	UObject *   pObject;
	UClass *    pClass;
	char *      pData;
	DWORD       ActorIndex;
};

struct UObjectProp
{
	UObject *   pObject;
	UProperty * pProperty;
	char *      pData;
};


HANDLE    hConsole;
HANDLE    hInputEvent;
char      Input[MaxFullNameSize];
char      Path[MAX_PATH];
bool      RestartLua;
HANDLE    StdErr;
HANDLE    StdIn;
HANDLE    StdOut;


int CloseConsole(lua_State * /*L*/) 
{
	if (hConsole)
	{
		SetStdHandle(STD_INPUT_HANDLE, StdIn);
		SetStdHandle(STD_OUTPUT_HANDLE, StdOut);
		SetStdHandle(STD_ERROR_HANDLE, StdErr);

		CloseHandle(hConsole);
		FreeConsole();

		hConsole = 0;
		SetEvent(hInputEvent);
	}

	return(0);
}

BOOL WINAPI ConsoleHandlerRoutine(DWORD	/*dwCtrlType*/)
{
	FreeConsole();
	return(1);
}

void DumpProperty(UProperty * pProperty, char * pData, char * pName)
{
	int Len;

	if (pProperty->IsA(pPropertyClass))
	{
		AppendType(pName, pProperty);

		strcat(pName, " ");
		Len = strlen(pName);
		if (Len < 40)
		{
			memset(&pName[Len], ' ', 40 - Len);
			pName[40] = 0;
		}

		pProperty->GetName(&pName[strlen(pName)]);

		if (pProperty->ElementCount > 1)
		{
			sprintf(&pName[strlen(pName)], "[%d]", 
				pProperty->ElementCount);
		}

		strcat(pName, " ");
		Len = strlen(pName);
		if (Len < 80)
		{
			memset(&pName[Len], ' ', 80 - Len);
			pName[80] = 0;
		}

		AppendValue(pName, pProperty, pData, 0);
	}
}

int Dump(lua_State * L) 
{
	int           Found;
	DWORD         Index;
	char          Name[MaxFullNameSize*8];
	char *        pData;
	UClass *      pClass;
	UObject *     pObject;
	UObjectData * pObjectData;
	UObjectProp * pObjectProp;
	UProperty *   pProperty;

	//Plain data type.
	if (!lua_touserdata(L, 1))
	{
		pData = Name;
		if (lua_isnoneornil(L, 1))
			pData = "[nil]";
		else if (lua_isnumber(L, 1))
			sprintf(pData, "[%0.3f]", (float)lua_tonumber(L, 1));
		else
			sprintf(pData, "[%s]", lua_tostring(L, 1));

		fputs("\n", stdout);
		fputs(pData, stdout);
		fputs("\n\n", stdout);
		Log("Dump: %s", pData);
	}

	//Check for UObject
	lua_getfield(L, LUA_REGISTRYINDEX, EngineUObject);
	lua_getmetatable(L, 1);
	Found = lua_rawequal(L, -1, -2);
	lua_pop(L, 2);
	if (Found)
	{
		pObjectData = (UObjectData *)luaL_checkudata(L, 1, EngineUObject);
		pObject   = pObjectData->pObject;
		pData     = pObjectData->pData;
		pClass    = pObjectData->pClass;

		if ((char *)pObject == pData)
		{
			pObject->GetFullName(Name);
		}
		else
		{
			pClass->GetFullName(Name);
			if (pObject)
			{
				strcat(Name, " part of [");
				pObject->GetFullName(&Name[strlen(Name)]);
				strcat(Name, "]");
			}
		}

		fputs("\n", stdout);
		fputs(Name, stdout);
		fputs("\n", stdout);
		Log("\n\n\nDump: %s", Name);

		while (pClass)
		{
			strcpy(Name, "  ");
			pClass->GetCPPName(&Name[strlen(Name)]);
			strcat(Name, "  [");
			pClass->GetFullName(&Name[strlen(Name)]);
			strcat(Name, "]");
			fputs(Name, stdout);
			fputs("\n", stdout);
			Log("Dump: %s", Name);

			pProperty = (UProperty *)pClass->pChildren;
			while (pProperty)
			{
				if (pProperty->IsA(pPropertyClass))
				{
					strcpy(Name, "    ");

					DumpProperty(pProperty, pData, Name);

					fputs(Name, stdout);
					fputs("\n", stdout);
					Log("Dump: %s", Name);
				}

				pProperty = (UProperty *)pProperty->pNext;
			}

			pClass = (UClass *)pClass->pSuper;
		}
		fputs("\n", stdout);

		return(0);
	}

	//Check for UProperty
	lua_getfield(L, LUA_REGISTRYINDEX, EngineUProperty);
	lua_getmetatable(L, 1);
	Found = lua_rawequal(L, -1, -2);
	lua_pop(L, 2);
	if (!Found)
		return(0);

	pObjectProp = (UObjectProp *)luaL_checkudata(L, 1, EngineUProperty);
	pObject     = pObjectProp->pObject;
	pProperty   = pObjectProp->pProperty;
	pData       = pObjectProp->pData;

	if ((char *)pObject == pData)
	{
		pObject->GetFullName(Name);
	}
	else
	{
		pProperty->GetFullName(Name);
		if (pObject)
		{
			strcat(Name, " part of [");
			pObject->GetFullName(&Name[strlen(Name)]);
			strcat(Name, "]");
		}
	}

	fputs("\n", stdout);
	fputs(Name, stdout);
	fputs("\n", stdout);
	Log("\n\n\nDump: %s", Name);

	if (pProperty->ElementCount > 1)
	{
		strcpy(Name, "  ");

		AppendType(Name, pProperty);
		strcat(Name, " ");
		pProperty->GetName(&Name[strlen(Name)]);
		sprintf(&Name[strlen(Name)], "[%d]", pProperty->ElementCount);

		fputs(Name, stdout);
		fputs("\n", stdout);
		Log("Dump: %s", Name);

		for (Index = 0; Index < pProperty->ElementCount; ++Index)
		{
			sprintf(Name, "    [%d] ", Index);
			AppendValue(Name, pProperty, pData, Index);

			fputs(Name, stdout);
			fputs("\n", stdout);
			Log("Dump: %s", Name);
		}
	}

	else if (pProperty->IsA(pStructPropertyClass))
	{
		pData += pProperty->CStructOffset;
		pClass = pProperty->pRelatedClass;
		while (pClass)
		{
			strcpy(Name, "  ");
			pClass->GetCPPName(&Name[strlen(Name)]);
			fputs(Name, stdout);
			fputs("\n", stdout);
			Log("Dump: %s", Name);

			pProperty = (UProperty *)pClass->pChildren;
			while (pProperty)
			{
				if (pProperty->IsA(pPropertyClass))
				{
					strcpy(Name, "    ");

					DumpProperty(pProperty, pData, Name);

					fputs(Name, stdout);
					fputs("\n", stdout);
					Log("Dump: %s", Name);
				}

				pProperty = (UProperty *)pProperty->pNext;
			}

			pClass = (UClass *)pClass->pSuper;
		}
	}

	else if (pProperty->IsA(pArrayPropertyClass))
	{
		TArray<void> * pArray;


		pArray = (TArray<void> *)(pData + pProperty->CStructOffset); 
		strcpy(Name, "  ");

		AppendType(Name, pProperty);
		strcat(Name, " ");
		pProperty->GetName(&Name[strlen(Name)]);
		sprintf(&Name[strlen(Name)], "[%d][%d]", pArray->Count, pArray->Max);

		fputs(Name, stdout);
		fputs("\n", stdout);
		Log("Dump: %s", Name);

		pProperty = (UProperty *)pProperty->pRelatedClass;
		for (Index = 0; Index < pArray->Count; ++Index)
		{
			sprintf(Name, "    [%d] ", Index);

			pData = (char *)((DWORD)pArray->pArray + (Index * pProperty->ElementSize));
			AppendValue(Name, pProperty, pData, 0);

			fputs(Name, stdout);
			fputs("\n", stdout);
			Log("Dump: %s", Name);
		}
	}

	else
	{
		pClass = (UClass *)pProperty;
		while (pClass)
		{
			strcpy(Name, "  ");
			pClass->GetCPPName(&Name[strlen(Name)]);
			strcat(Name, "  [");
			pClass->GetFullName(&Name[strlen(Name)]);
			strcat(Name, "]");
			fputs(Name, stdout);
			fputs("\n", stdout);
			Log("Dump: %s", Name);

			pProperty = (UProperty *)pClass->pChildren;
			while (pProperty)
			{
				if (pProperty->IsA(pPropertyClass))
				{
					strcpy(Name, "    ");

					DumpProperty(pProperty, pData, Name);

					fputs(Name, stdout);
					fputs("\n", stdout);
					Log("Dump: %s", Name);
				}

				pProperty = (UProperty *)pProperty->pNext;
			}

			pClass = (UClass *)pClass->pSuper;
		}
	}
	fputs("\n", stdout);

	return(0);
}


int DumpClass(lua_State * L) 
{
	int           Found;
	char          Name[MaxFullNameSize*8];
	UClass *      pClass;
	UObjectData * pObjectData;
	UObjectProp * pObjectProp;

	//Plain data type.
	if (!lua_touserdata(L, 1))
		luaL_argcheck(L, 0, 1, "Expecting a UObject or UProperty.");

	//Check for UObject
	lua_getfield(L, LUA_REGISTRYINDEX, EngineUObject);
	lua_getmetatable(L, 1);
	Found = lua_rawequal(L, -1, -2);
	lua_pop(L, 2);
	if (Found)
	{
		pObjectData = (UObjectData *)luaL_checkudata(L, 1, EngineUObject);
		if (!pObjectData->pObject)
			luaL_argcheck(L, 0, 1, "Expecting a UObject or UProperty.");

		pClass = pObjectData->pClass;
		pClass->GetFullName(Name);
		pClass = (UClass *)pClass->pSuper;

		if (pObjectData->pObject && pObjectData->pObject->IsA(pClassClass))
		{
			strcat(Name, " [");
			pObjectData->pObject->GetFullName(&Name[strlen(Name)]);
			strcat(Name, "]");
		}
	}

	//Check for UProperty
	else
	{
		lua_getfield(L, LUA_REGISTRYINDEX, EngineUProperty);
		lua_getmetatable(L, 1);
		Found = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (!Found)
			return(0);

		pObjectProp = (UObjectProp *)luaL_checkudata(L, 1, EngineUProperty);
		pObjectProp->pProperty->GetFullName(Name);

		pClass = 0;
		if (pObjectProp->pProperty->IsA(pArrayPropertyClass) ||
			pObjectProp->pProperty->IsA(pStructPropertyClass))
		{
			pClass = pObjectProp->pProperty->pRelatedClass;
		}
	}

	while (pClass)
	{
		strcat(Name, "\n  ");
		pClass->GetFullName(&Name[strlen(Name)]);

		pClass = (UClass *)pClass->pSuper;
	}

	fputs(Name, stdout);
	fputs("\n", stdout);
	Log("DumpClass: %s", Name);

	return(0);
}

int GetKeyState(lua_State * L) {
  if (lua_isnumber(L, 1)) {
    if (GetAsyncKeyState(lua_tonumber(L, 1)) & 0x8000) {
      lua_pushboolean(L, true);
    } else {
      lua_pushboolean(L, false);
    }
  }
  return(1);
}

int FindFirst(lua_State * L) 
{
	FName         Name;
	UClass *      pClass;
	UObject *     pObject;
	UObjectData * pObjectData;


	if (lua_type(L, 1) == LUA_TUSERDATA)
	{
		pObjectData = (UObjectData *)luaL_checkudata(L, 1, EngineUObject);
		if ((char *)pObjectData->pObject != pObjectData->pData)
			luaL_argcheck(L, 0, 1, "Expecting a UObject or FName, UClass");

		pObject = pObjectData->pObject;
		pClass  = pObject->pClass;
		Name    = pObject->Name;
	}
	else
	{
		Name = luaL_checkinteger(L, 1);
		pObjectData = (UObjectData *)luaL_checkudata(L, 2, EngineUObject);
		if ((char *)pObjectData->pObject != pObjectData->pData)
			luaL_argcheck(L, 0, 2, "Expecting a UObject or FName, UClass");
		if (!pObjectData->pObject->IsA(pClassClass))
			luaL_argcheck(L, 0, 2, "Expecting a UObject or FName, UClass");

		pClass = (UClass *)pObjectData->pObject;
	}

	//pObject = pUObjectHash[Name & 0xFFF];
	pObject = pUObjectHash[Name & 0x1FFF];
	while (pObject && !pObject->IsA(pClass))
		pObject = pObject->pHash;

	if (pObject)
	{
		pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
		pObjectData->pObject    = pObject;
		pObjectData->pClass     = pObject->pClass;
		pObjectData->pData      = (char *)pObject;
		pObjectData->ActorIndex = (DWORD)-1;

		luaL_getmetatable(L, EngineUObject);
		lua_setmetatable(L, -2);
	}
	else
	{
		lua_pushnil(L);
	}

	return(1);
}


int FindFirstAActor(lua_State * L) 
{
	DWORD         Index;
	UClass *      pClass;
	UObject *     pObject;
	UObjectData * pObjectData;

	pObjectData = (UObjectData *)luaL_checkudata(L, 1, EngineUObject);
	if ((char *)pObjectData->pObject != pObjectData->pData)
		luaL_argcheck(L, 0, 1, "Expecting a UClass");
	if (!pObjectData->pObject->IsA(pClassClass))
		luaL_argcheck(L, 0, 1, "Expecting a UClass");

	UWorld* pWorld = *(UWorld **)0x1DEE5EC;

	pAActorArray = &pWorld->Levels.pArray[0]->ActorArray;

	pClass = (UClass *)pObjectData->pObject;

	Index = 0;
	pObject = 0;

	while (Index < pAActorArray->Count)
	{
		if(!pAActorArray->pArray[Index])
		{
			++Index;
			continue;
		}

		pObject = pAActorArray->pArray[Index];

		if (pObject && (pObject->IsA(pClass)))
			break;

		++Index;
	}

	if (pObject && (Index < pAActorArray->Count))
	{
		pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
		pObjectData->pObject    = pObject;
		pObjectData->pClass     = pObject->pClass;
		pObjectData->pData      = (char *)pObject;
		pObjectData->ActorIndex = Index;

		luaL_getmetatable(L, EngineUObject);
		lua_setmetatable(L, -2);
	}
	else
	{
		lua_pushnil(L);
	}

	return(1);
}


int FindNext(lua_State * L) 
{
	UClass *      pClass;
	UObject *     pObject;
	UObjectData * pObjectData;

	pObjectData = (UObjectData *)luaL_checkudata(L, 1, EngineUObject);
	if ((char *)pObjectData->pObject != pObjectData->pData)
		luaL_argcheck(L, 0, 1, "Expecting a UObject");

	pObject = pObjectData->pObject;
	pClass = pObject->pClass;

	pObject = pObject->pHash;
	while (pObject && (pObject->pClass != pClass))
		pObject = pObject->pHash;

	if (pObject)
	{
		pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
		pObjectData->pObject    = pObject;
		pObjectData->pClass     = pObject->pClass;
		pObjectData->pData      = (char *)pObject;
		pObjectData->ActorIndex = (DWORD)-1;

		luaL_getmetatable(L, EngineUObject);
		lua_setmetatable(L, -2);
	}
	else
	{
		lua_pushnil(L);
	}

	return(1);
}


int FindNextAActor(lua_State * L) 
{
	DWORD         Index;
	UClass *      pClass;
	UObject *     pObject;
	UObjectData * pObjectData;

	pObjectData = (UObjectData *)luaL_checkudata(L, 1, EngineUObject);
	if ((char *)pObjectData->pObject != pObjectData->pData)
		luaL_argcheck(L, 0, 1, "Expecting a UObject, UClass");

	Index = pObjectData->ActorIndex + 1;

	pObjectData = (UObjectData *)luaL_checkudata(L, 2, EngineUObject);
	if ((char *)pObjectData->pObject != pObjectData->pData)
		luaL_argcheck(L, 0, 2, "Expecting a UObject, UClass");
	if (!pObjectData->pObject->IsA(pClassClass))
		luaL_argcheck(L, 0, 2, "Expecting a UObject, UClass");

	pClass = (UClass *)pObjectData->pObject;

	pObject = 0;

	UWorld* pWorld = *(UWorld **)0x1DEE5EC;
	pAActorArray = &pWorld->Levels.pArray[0]->ActorArray;

	while (Index < pAActorArray->Count)
	{
		if(!pAActorArray->pArray[Index])
		{
			++Index;
			continue;
		}

		pObject = pAActorArray->pArray[Index];
		if (pObject && (pObject->IsA(pClass)))
			break;

		++Index;
	}

	if (pObject && (Index < pAActorArray->Count))
	{
		pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
		pObjectData->pObject    = pObject;
		pObjectData->pClass     = pObject->pClass;
		pObjectData->pData      = (char *)pObject;
		pObjectData->ActorIndex = Index;

		luaL_getmetatable(L, EngineUObject);
		lua_setmetatable(L, -2);
	}
	else
	{
		lua_pushnil(L);
	}

	return(1);
}


int FullName(lua_State * L) 
{
	int             Found;
	char            Name[MaxFullNameSize];
	UObjectData *   pObjectData;
	UFunctionData * pFunctionData;


	lua_getfield(L, LUA_REGISTRYINDEX, EngineUObject);
	lua_getmetatable(L, 1);
	Found = lua_rawequal(L, -1, -2);
	lua_pop(L, 2);
	if (Found)
	{
		pObjectData = (UObjectData *)luaL_checkudata(L, 1, EngineUObject);
		if ((char *)pObjectData->pObject != pObjectData->pData)
			luaL_argcheck(L, 0, 1, "Expecting a UObject or UFunction");
		pObjectData->pObject->GetFullName(Name);
	}
	else
	{
		pFunctionData = (UFunctionData *)luaL_checkudata(L, 1, EngineUFunction);
		pFunctionData->pFunction->GetFullName(Name);
	}

	lua_pushstring(L, Name);

	return(1);
}


unsigned int __stdcall InputThread(void *)
{
	Input[0] = 0;
	ResetEvent(hInputEvent);
	while (hConsole)
	{
		fgets(&Input[1], sizeof(Input) - 1, stdin);
		Input[0] = 1;
		WaitForSingleObject(hInputEvent, INFINITE);
	}

	return(0);
}


int IsA(lua_State * L) 
{
	UClass *      pClass;
	UObjectData * pClassData;
	UObjectData * pObjectData;


	pObjectData = (UObjectData *)luaL_checkudata(L, 1, EngineUObject);
	if ((char *)pObjectData->pObject != pObjectData->pData)
		luaL_argcheck(L, 0, 1, "Expecting a UObject, UClass");

	pClassData = (UObjectData *)luaL_checkudata(L, 2, EngineUObject);
	if ((char *)pClassData->pObject != pClassData->pData)
		luaL_argcheck(L, 0, 2, "Expecting a UObject, UClass");
	if (!pClassData->pObject->IsA(pClassClass))
		luaL_argcheck(L, 0, 2, "Expecting a UObject, UClass");

	pClass = (UClass *)pClassData->pObject;

	lua_pushboolean(L, pObjectData->pObject->IsA(pClass));


	return(1);
}


//extern HWND GetConsoleWindow(void);
int OpenConsole(lua_State *	/*L*/) 
{
	int          ConsoleHandle;
	FILE *       File;
	unsigned int ID;


	if (hConsole)
		return(0);

	AllocConsole();
	SetConsoleTitleA("Command Prompt");
	SetConsoleCtrlHandler(ConsoleHandlerRoutine, 1);


	//Fix up stdin/stdout/stderr
	StdIn = GetStdHandle(STD_INPUT_HANDLE);
	ConsoleHandle = _open_osfhandle((long)StdIn, _O_TEXT);
	File = _fdopen(ConsoleHandle, "r");
	*stdin = *File;
	setvbuf(stdin, NULL, _IONBF, 0);

	StdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	ConsoleHandle = _open_osfhandle((long)StdOut, _O_TEXT);
	File = _fdopen(ConsoleHandle, "w");
	*stdout = *File;
	setvbuf(stdout, NULL, _IONBF, 0);

	StdErr = GetStdHandle(STD_ERROR_HANDLE);
	ConsoleHandle = _open_osfhandle((long)StdErr, _O_TEXT);
	File = _fdopen(ConsoleHandle, "w");
	*stdout = *File;
	setvbuf(stdout, NULL, _IONBF, 0);

	fputs("\n> ", stdout);
	hConsole = (HANDLE)_beginthreadex(0, 0, InputThread, 0, 0, &ID);

	return(0);
}


int Load(lua_State * L) 
{
	char * pFileName;
	char   Script[MAX_PATH];

	strcpy(Script, Path);
	pFileName = (char *)luaL_checkstring(L, 1);
	strcat(Script, pFileName);

	fputs("Loading[", stdout);
	fputs(Script, stdout);
	fputs("]\n", stdout);
	Log("System: Loading[%s]", Script);
	if (luaL_dofile(L, Script))
	{
		const char * pError = lua_tostring(L, -1);

		fputs("Script Error: ", stdout);
		if (pError)
		{
			fputs(pError, stdout);
			Log("Script Error: %s", pError);
		}
		else
		{
			fputs("[unknown]", stdout);
			Log("Script Error: [unknown]");
		}

		fputs("\n", stdout);
	}
	fputs("\n", stdout);

	return(0);
}


int LogString(lua_State * L) 
{
	//Log("Script: %s", (char *)luaL_checkstring(L, 1));
	Log("%s", (char *)luaL_checkstring(L, 1));
	return(0);
}


int New(lua_State * L) 
{
	UObjectData * pObjectData;
	UObjectData * pNewData;
	UStruct *     pStruct;

	pObjectData = (UObjectData *)luaL_checkudata(L, 1, EngineUObject);

	if (!pObjectData->pObject->IsA(pStructClass))
		luaL_argcheck(L, 0, 1, "Expecting a Struct.");

	pStruct = (UStruct *)pObjectData->pObject;

	pNewData = (UObjectData *)lua_newuserdata(L, sizeof(*pNewData) +
		pStruct->Size);
	pNewData->pObject = 0;
	pNewData->pClass  = (UClass *)pStruct;
	pNewData->pData   = (char *)pNewData + sizeof(*pNewData);

	luaL_getmetatable(L, EngineUObject);
	lua_setmetatable(L, -2);

	memset(pNewData->pData, 0, pStruct->Size);

	return(1);
}


int Restart(lua_State *	/*L*/) 
{
	RestartLua = true;
	return(0);
}


/******************************************************************************
*
* Lua Common Functions to find UScript properties.
*
******************************************************************************/
#define MaxPropCache 256

struct PropertyCacheStruct
{
	UClass *    pClass;
	UProperty * pProperty;
	char        Name[MaxNameSize];
};

PropertyCacheStruct * Cache[MaxPropCache];
PropertyCacheStruct   NewCacheData[MaxPropCache]; //fixed


void ResetPropertyCache(void)
{
	int Index;

	for (Index = 0; Index < MaxPropCache; ++Index)
		Cache[Index] = &NewCacheData[Index];

	memset(NewCacheData, 0, sizeof(NewCacheData));
}

UProperty * FindPropertySlow(UClass * pClass, const char * pPropertyName)
{
	UField *    pField;
	UProperty * pProperty;
	WCHAR       WName[MaxNameSize];

	wsprintfW(WName, L"%S", pPropertyName);

	pProperty = 0;
	while (pClass && !pProperty)
	{
		pField = pClass->pChildren;
		while (pField)
		{
			if (pField->pClass == pFunctionClass)
			{
				if (!wcscmp(WName, GetFName(pField->Name)))
				{
					pProperty = (UProperty *)pField;
					break;
				}
			}

			else if (pField->IsA(pPropertyClass))
			{
				if (!wcscmp(WName, GetFName(pField->Name)))
				{
					pProperty = (UProperty *)pField;
					break;
				}
			}

			pField = pField->pNext;
		}

		pClass = (UClass *)pClass->pSuper;
	}

	return(pProperty);
}


UProperty * FindProperty(lua_State * L, UClass * pClass, const char * pPropertyName)
{
	int                    Index;
    PropertyCacheStruct *  pCache;
	UProperty *            pProperty;
    PropertyCacheStruct ** ppCache;

	//Search the cache.
	ppCache = Cache;
	for (Index = 0; Index < MaxPropCache; ++Index)
	{
		if ((*ppCache)->pClass &&
			((*ppCache)->pClass == pClass) &&
			!strcmp((*ppCache)->Name, pPropertyName))
		{
			break;
		}
		++ppCache;
	}


	//We don't have a cache hit, so look it up the old way.
	if ((Index >= MaxPropCache) || !(*ppCache)->pClass)
	{
		pProperty = FindPropertySlow(pClass, pPropertyName);
		if (!pProperty)
		{
			char Line[512];
			sprintf(Line, "[%s] is not a valid property of [%S]",
				pPropertyName, GetFName(pClass->Name));
			luaL_argcheck(L, pProperty, 2, Line);
			return(0);
		}

		Log("System: Caching Property[%s] Class[%S]", 
			pPropertyName, GetFName(pClass->Name));

		Index = MaxPropCache - 1;
 		pCache = Cache[Index];
 		pCache->pClass = pClass;
 		pCache->pProperty = pProperty;
 		strcpy(pCache->Name, pPropertyName);
	}


	//Move the item to the top.
	pCache = Cache[Index];
	memmove(&Cache[1], &Cache[0], sizeof(Cache[0]) * Index);
	Cache[0] = pCache;


	return(pCache->pProperty);
}


/******************************************************************************
*
* Lua Common Functions to Get/Set properties and to create
* parameter blocks for calling UScript functions.
*
******************************************************************************/
void CreateReturn(lua_State * L, UProperty * pProperty, char * pParam)
{
	if (pProperty->IsA(pBoolPropertyClass))
	{
		lua_pushboolean(L, *(bool *)pParam);
	}

	else if (pProperty->IsA(pBytePropertyClass))
	{
		lua_pushinteger(L, *(BYTE *)pParam);
	}

	else if (pProperty->IsA(pClassPropertyClass) ||
		pProperty->IsA(pObjectPropertyClass) ||
    		pProperty->IsA(pInterfacePropertyClass))
	{
		UObjectData * pObjectData;


		pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
		pObjectData->pObject    = *(UClass **)pParam;
		pObjectData->pClass     = pObjectData->pObject->pClass;
		pObjectData->pData      = (char *)pObjectData->pObject;
		pObjectData->ActorIndex = (DWORD)-1;

		luaL_getmetatable(L, EngineUObject);
		lua_setmetatable(L, -2);
	}

	else if (pProperty->IsA(pFloatPropertyClass))
	{
		lua_pushnumber(L, *(float *)pParam);
	}

	else if (pProperty->IsA(pIntPropertyClass))
	{
		lua_pushinteger(L, *(int *)pParam);
	}

	else if (pProperty->IsA(pNamePropertyClass))
	{
		lua_pushinteger(L, *(FName *)pParam);
	}

	else if (pProperty->IsA(pStrPropertyClass))
	{
		TArray<WCHAR> * Value;

		Value = (TArray<WCHAR> *)pParam;
		if (Value->pArray)
		{
			char * pName = new char[Value->Count];
			sprintf(pName, "%S", Value->pArray);
			lua_pushstring(L, pName);
			delete[] pName;
		}
		else
		{
			lua_pushnil(L);
		}
	}

	else if (pProperty->IsA(pStructPropertyClass))
	{
		UObjectData * pObjectData;


		pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData) +
			pProperty->ElementSize);
		pObjectData->pObject = 0;
		pObjectData->pClass  = pProperty->pRelatedClass;
		pObjectData->pData   = (char *)pObjectData + sizeof(*pObjectData);

		luaL_getmetatable(L, EngineUObject);
		lua_setmetatable(L, -2);

		memcpy(pObjectData->pData, pParam, pProperty->ElementSize);
	}

	else
	{
		luaL_argcheck(L, 0, 0, "Return property type not implemented.");
	}
}


void GetProperty(lua_State * L, UObject * pObject, UProperty * pProperty, 
	char * pData, int Index)
{
	UObjectData *   pObjectData;
	UObjectProp *   pObjectProp;
	UFunctionData * pFunctionData;


	if (pProperty->IsA(pFunctionClass))
	{
		pFunctionData = (UFunctionData *)lua_newuserdata(L, sizeof(*pFunctionData));
		pFunctionData->pObject   = pObject;
		pFunctionData->pFunction = (UFunction *)pProperty;

		luaL_getmetatable(L, EngineUFunction);
		lua_setmetatable(L, -2);
		return;
	}

	if ((Index == NoIndex) && ((pProperty->ElementCount > 1) || 
		pProperty->IsA(pArrayPropertyClass)))
	{
		pObjectProp = (UObjectProp *)lua_newuserdata(L, sizeof(*pObjectProp));
		pObjectProp->pObject   = pObject;
		pObjectProp->pProperty = pProperty;
		pObjectProp->pData     = pData;

		luaL_getmetatable(L, EngineUProperty);
		lua_setmetatable(L, -2);
		return;
	}

  	//if its not an array, only then if elecount is 1 we have a problem
	if ((Index != NoIndex) && (pProperty->ElementCount == 1) && !pProperty->IsA(pArrayPropertyClass))
	{
		luaL_argcheck(L, 0, 2, "Property cannot be indexed.");
		return;
	}

	if (Index == NoIndex)
		Index = 0;
 	//if its not an array, only then if elecount is 1 we have a problem
	if (((Index < 0) || ((DWORD)Index >= pProperty->ElementCount))
     		&& !pProperty->IsA(pArrayPropertyClass))
	{
		luaL_argcheck(L, 0, 2, "Index out of range.");
		return;
	}


	if (pProperty->IsA(pArrayPropertyClass))
	{
		TArray<void> * pValue;

		pValue = (TArray<void> *)(pData + pProperty->CStructOffset); 
		if ((DWORD)Index >= pValue->Count)
		{
			luaL_argcheck(L, 0, 2, "Index out of range.");
			return;
		}

		pProperty = (UProperty *)pProperty->pRelatedClass;
		GetProperty(L, pObject, pProperty, (char *)((DWORD)pValue->pArray + 
			(Index * pProperty->ElementSize)), NoIndex);
	}

	else if (pProperty->IsA(pBoolPropertyClass))
	{
		int Value;

		Value = *(int *)(pData + pProperty->CStructOffset + 
			(pProperty->ElementSize * Index));
		Value &= (DWORD)pProperty->pRelatedClass;
		if (Value)
			Value = 1;
		lua_pushboolean(L, Value);
	}

	else if (pProperty->IsA(pBytePropertyClass))
	{
		int Value;

		Value = *(BYTE *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		lua_pushinteger(L, Value);
	}

	else if (pProperty->IsA(pClassPropertyClass) ||
		pProperty->IsA(pObjectPropertyClass) ||
    		pProperty->IsA(pInterfacePropertyClass))
	{
		UObject * pValue;

		pValue = *(UObject **)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));

		if (pValue)
		{
			pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
			pObjectData->pObject    = pValue;
			pObjectData->pClass     = pValue->pClass;
			pObjectData->pData      = (char *)pValue;
			pObjectData->ActorIndex = (DWORD)-1;

			luaL_getmetatable(L, EngineUObject);
			lua_setmetatable(L, -2);
		}
		else
		{
			lua_pushnil(L);
		}
	}

	else if (pProperty->IsA(pFloatPropertyClass))
	{
		float Value;

		Value = *(float *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		lua_pushnumber(L, Value);
	}

	else if (pProperty->IsA(pIntPropertyClass))
	{
		int Value;

		Value = *(int *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		lua_pushinteger(L, Value);
	}

	else if (pProperty->IsA(pNamePropertyClass))
	{
		FName Value;

		Value = *(FName *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		lua_pushinteger(L, Value);
	}

	else if (pProperty->IsA(pPointerPropertyClass))
	{
		int Value;

		Value = *(int *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		lua_pushinteger(L, Value);
	}

	else if (pProperty->IsA(pStrPropertyClass))
	{
		TArray<WCHAR> * Value;

		Value = (TArray<WCHAR> *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		if (Value->pArray)
		{
			char * pName = new char[Value->Count];
			sprintf(pName, "%S", Value->pArray);
			lua_pushstring(L, pName);
			delete[] pName;
		}
		else
		{
			lua_pushnil(L);
		}
	}

	else if (pProperty->IsA(pStructPropertyClass))
	{
		pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
		pObjectData->pObject    = pObject;
		pObjectData->pClass     = pProperty->pRelatedClass;
		pObjectData->pData      = pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index);
		pObjectData->ActorIndex = (DWORD)-1;

		luaL_getmetatable(L, EngineUObject);
		lua_setmetatable(L, -2);
	}

	//pDelegatePropertyClass
	//pFixedArrayPropertyClass
	//pMapPropertyClass
	else
	{
		luaL_argcheck(L, 0, 2, "Can't get this property type.");
	}
}


void InParam(lua_State * L, UProperty * pProperty, int ParamIndex, char * pParam)
{
	if (pProperty->IsA(pBoolPropertyClass))
	{
		int Value;

		luaL_checktype(L, ParamIndex, LUA_TBOOLEAN);
		Value = lua_toboolean(L, ParamIndex);
		*(int *)pParam = Value;
	}

	else if (pProperty->IsA(pBytePropertyClass))
	{
		int Value;

		Value = luaL_checkinteger(L, ParamIndex);
		*(BYTE *)pParam = (BYTE)Value;
	}


	else if (pProperty->IsA(pClassPropertyClass) ||
		pProperty->IsA(pObjectPropertyClass) ||
    		pProperty->IsA(pInterfacePropertyClass))
	{
		int           Found;
		UObjectData * pObjectData;
		UObjectProp * pObjectProp;


		lua_getfield(L, LUA_REGISTRYINDEX, EngineUObject);
		lua_getmetatable(L, ParamIndex);
		Found = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (Found)
		{
			pObjectData = (UObjectData *)luaL_checkudata(L, ParamIndex, EngineUObject);
			*(UClass **)pParam = (UClass *)pObjectData->pData;

			return;
		}

		lua_getfield(L, LUA_REGISTRYINDEX, EngineUProperty);
		lua_getmetatable(L, ParamIndex);
		Found = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (!Found)
			return;

		pObjectProp = (UObjectProp *)luaL_checkudata(L, ParamIndex, EngineUProperty);
		memcpy(pParam, pObjectProp->pData, 4);
	}

	else if (pProperty->IsA(pFloatPropertyClass))
	{
		float Value;

		Value = (float)luaL_checknumber(L, ParamIndex);
		*(float *)pParam = Value;
	}

	else if (pProperty->IsA(pIntPropertyClass))
	{
		int Value;

		Value = luaL_checkinteger(L, ParamIndex);
		*(int *)pParam = Value;
	}

	else if (pProperty->IsA(pNamePropertyClass))
	{
		FName Value;

		Value = luaL_checkinteger(L, ParamIndex);
		*(FName *)pParam = Value;
	}

	else if (pProperty->IsA(pStrPropertyClass))
	{
		const char * pValue;

		pValue = luaL_checkstring(L, ParamIndex);
		FStringConstructor(pParam, pValue);
	}

	else if (pProperty->IsA(pStructPropertyClass))
	{
		int           Found;
		UObjectData * pObjectData;
		UObjectProp * pObjectProp;


		lua_getfield(L, LUA_REGISTRYINDEX, EngineUObject);
		lua_getmetatable(L, ParamIndex);
		Found = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (Found)
		{
			pObjectData = (UObjectData *)luaL_checkudata(L, ParamIndex, EngineUObject);
			memcpy(pParam, pObjectData->pData, pObjectData->pClass->Size);

			return;
		}

		lua_getfield(L, LUA_REGISTRYINDEX, EngineUProperty);
		lua_getmetatable(L, ParamIndex);
		Found = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (!Found)
			return;

		pObjectProp = (UObjectProp *)luaL_checkudata(L, ParamIndex, EngineUProperty);
		memcpy(pParam, pObjectProp->pData, pProperty->ElementSize);
	}
}


void SetProperty(lua_State * L, UProperty * pProperty, char * pData, int Index)
{
	if ((Index == NoIndex) && (pProperty->ElementCount > 1))
	{
		luaL_argcheck(L, 0, 2, "Can't set multiple array elements.");
		return;
	}

	if ((Index != NoIndex) && (pProperty->ElementCount == 1) &&
		!pProperty->IsA(pArrayPropertyClass))
	{
		luaL_argcheck(L, 0, 2, "Property cannot be indexed.");
		return;
	}

	if (Index == NoIndex)
		Index = 0;

	if (((Index < 0) || ((DWORD)Index >= pProperty->ElementCount))
    		&& !pProperty->IsA(pArrayPropertyClass))
	{
		luaL_argcheck(L, 0, 2, "Index out of range.");
		return;
	}

	if (pProperty->IsA(pArrayPropertyClass))
	{
		TArray<void> * pArray;

		pArray = (TArray<void> *)(pData + pProperty->CStructOffset); 
		if ((DWORD)Index >= pArray->Count)
		{
			luaL_argcheck(L, 0, 2, "Index out of range.");
			return;
		}

		pProperty = (UProperty *)pProperty->pRelatedClass;
		SetProperty(L, pProperty, (char *)((DWORD)pArray->pArray + 
			(Index * pProperty->ElementSize)), NoIndex);
	}

	else if (pProperty->IsA(pBoolPropertyClass))
	{
		int Value;


		Value = (DWORD)pProperty->pRelatedClass;
		luaL_checktype(L, 3, LUA_TBOOLEAN);
		if (lua_toboolean(L, 3))
		{
			*(int *)(pData + pProperty->CStructOffset + 
				(pProperty->ElementSize * Index)) |= Value;
		}

		else
		{
			*(int *)(pData + pProperty->CStructOffset + 
				(pProperty->ElementSize * Index)) &= ~Value;
		}
	}

	else if (pProperty->IsA(pBytePropertyClass))
	{
		BYTE Value;

		Value = (BYTE)luaL_checkinteger(L, 3);
		*(BYTE *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index)) = Value;
	}

	else if (pProperty->IsA(pClassPropertyClass) ||
	    pProperty->IsA(pObjectPropertyClass) ||
      	    pProperty->IsA(pInterfacePropertyClass))
	{
		UObjectData * pValue;

		pValue = (UObjectData *)luaL_checkudata(L, 3, EngineUObject);
		*(UClass **)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index)) = (UClass *)pValue->pData;
	}

	else if (pProperty->IsA(pFloatPropertyClass))
	{
		float Value;

		Value = (float)luaL_checknumber(L, 3);
		*(float *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index)) = Value;
	}

	else if (pProperty->IsA(pIntPropertyClass))
	{
		int Value;

		Value = luaL_checkinteger(L, 3);
		*(int *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index)) = Value;
	}

	else if (pProperty->IsA(pNamePropertyClass))
	{
		FName Value;

		Value = (FName)luaL_checkinteger(L, 3);
		*(FName *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index)) = Value;
	}

	else if (pProperty->IsA(pPointerPropertyClass))
	{
		int Value;

		Value = luaL_checkinteger(L, 3);
		*(int *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index)) = Value;
	}

	else if (pProperty->IsA(pStrPropertyClass))
	{
		TArray<WCHAR> * Value;

		Value = (TArray<WCHAR> *)(pData + pProperty->CStructOffset +
			(pProperty->ElementSize * Index));
		if (Value->pArray)
			FStringDestructor(Value);

		if (!lua_isnil(L, 3))
			FStringConstructor(Value, luaL_checkstring(L, 3));
	}

	else if (pProperty->IsA(pStructPropertyClass))
	{
		UObjectData * pValue;

		pValue = (UObjectData *)luaL_checkudata(L, 3, EngineUObject);
		memcpy(pData + pProperty->CStructOffset + 
			(pProperty->ElementSize * Index), pValue->pData, 
			pProperty->ElementSize);
	}

	//pDelegatePropertyClass
	//pFixedArrayPropertyClass
	//pMapPropertyClass
	else
	{
		luaL_argcheck(L, 0, 2, "Can't set this property type.");
	}
}

void ValidateParam(lua_State * L, UProperty * pProperty, int ParamIndex)
{
	if (pProperty->IsA(pBoolPropertyClass))
	{
		luaL_checktype(L, ParamIndex, LUA_TBOOLEAN);
	}

	else if (pProperty->IsA(pBytePropertyClass))
	{
		luaL_checkinteger(L, ParamIndex);
	}

	else if (pProperty->IsA(pClassPropertyClass))
	{
		int           Found;
		UObjectData * pObjectData;
		UObjectProp * pObjectProp;


		lua_getfield(L, LUA_REGISTRYINDEX, EngineUObject);
		lua_getmetatable(L, ParamIndex);
		Found = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (Found)
		{
			pObjectData = (UObjectData *)luaL_checkudata(L, ParamIndex, EngineUObject);
			if (!pObjectData->pObject || 
				!pObjectData->pObject->IsA(pClassClass) ||
				((char *)pObjectData->pObject != pObjectData->pData))
			{
				luaL_argcheck(L, 0, ParamIndex, "Expecting a UClass.");
			}

			return;
		}

		lua_getfield(L, LUA_REGISTRYINDEX, EngineUProperty);
		lua_getmetatable(L, ParamIndex);
		Found = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (!Found)
			return;

		pObjectProp = (UObjectProp *)luaL_checkudata(L, ParamIndex, EngineUProperty);
		if (!pObjectProp->pProperty->IsA(pClassPropertyClass))
			luaL_argcheck(L, 0, ParamIndex, "Expecting a Class.");
	}

	else if (pProperty->IsA(pFloatPropertyClass))
	{
		luaL_checknumber(L, ParamIndex);
	}

	else if (pProperty->IsA(pIntPropertyClass))
	{
		luaL_checkinteger(L, ParamIndex);
	}

	else if (pProperty->IsA(pNamePropertyClass))
	{
		luaL_checkinteger(L, ParamIndex);
	}

	else if (pProperty->IsA(pObjectPropertyClass)
    		|| pProperty->IsA(pInterfacePropertyClass))
	{
		int           Found;
		UObjectData * pObjectData;
		UObjectProp * pObjectProp;

		lua_getfield(L, LUA_REGISTRYINDEX, EngineUObject);
		lua_getmetatable(L, ParamIndex);
		Found = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (Found)
		{
			pObjectData = (UObjectData *)luaL_checkudata(L, ParamIndex, EngineUObject);
			if (!pObjectData->pObject || 
				((char *)pObjectData->pObject != pObjectData->pData))
			{
				luaL_argcheck(L, 0, ParamIndex, "Expecting a UObject.");
			}

			return;
		}

		lua_getfield(L, LUA_REGISTRYINDEX, EngineUProperty);
		lua_getmetatable(L, ParamIndex);
		Found = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (!Found)
			return;

		pObjectProp = (UObjectProp *)luaL_checkudata(L, ParamIndex, EngineUProperty);
		if (!pObjectProp->pProperty->IsA(pObjectPropertyClass))
			luaL_argcheck(L, 0, ParamIndex, "Expecting a UObject.");
	}

	else if (pProperty->IsA(pStrPropertyClass))
	{
		luaL_checkstring(L, ParamIndex);
	}

	else if (pProperty->IsA(pStructPropertyClass))
	{
		int           Found;
		UObjectData * pObjectData;
		UObjectProp * pObjectProp;


		lua_getfield(L, LUA_REGISTRYINDEX, EngineUObject);
		lua_getmetatable(L, ParamIndex);
		Found = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (Found)
		{
			pObjectData = (UObjectData *)luaL_checkudata(L, ParamIndex, EngineUObject);
			if (!pObjectData->pClass->IsA(pStructClass))
				luaL_argcheck(L, 0, ParamIndex, "Expecting a Struct.");

			return;
		}

		lua_getfield(L, LUA_REGISTRYINDEX, EngineUProperty);
		lua_getmetatable(L, ParamIndex);
		Found = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (!Found)
			return;

		pObjectProp = (UObjectProp *)luaL_checkudata(L, ParamIndex, EngineUProperty);
		if (!pObjectProp->pProperty->IsA(pStructPropertyClass))
			luaL_argcheck(L, 0, ParamIndex, "Expecting a Struct.");
	}

	//pArrayPropertyClass
	//pDelegatePropertyClass
	//pFixedArrayPropertyClass
	//pMapPropertyClass
	//pPointerPropertyClass
	else
	{
		luaL_argcheck(L, 0, ParamIndex, "Property type not implemented.");
	}
}


/******************************************************************************
*
* Lua AActors Functions
*
******************************************************************************/
int AActorsIndex(lua_State * L) 
{
	DWORD         Index;
	UObject *     pObject;
	UObjectData * pObjectData;


	if (lua_type(L, 2) == LUA_TSTRING)
	{
		pObject = FindActorByFullName(luaL_checkstring(L, 2));
		if (pObject)
		{
			pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
			pObjectData->pObject    = pObject;
			pObjectData->pClass     = pObject->pClass;
			pObjectData->pData      = (char *)pObject;
			pObjectData->ActorIndex = (DWORD)-1;

			luaL_getmetatable(L, EngineUObject);
			lua_setmetatable(L, -2);
		}
		else
		{
			lua_pushnil(L);
		}
	}

	else
	{
		Index = luaL_checkinteger(L, 2);
		if (Index < pAActorArray->Count)
		{
			if (pAActorArray->pArray[Index])
			{
				pObject = pAActorArray->pArray[Index];

				pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
				pObjectData->pObject    = pObject;
				pObjectData->pClass     = pObject->pClass;
				pObjectData->pData      = (char *)pObject;
				pObjectData->ActorIndex = (DWORD)-1;

				luaL_getmetatable(L, EngineUObject);
				lua_setmetatable(L, -2);
			}
			else
			{
				lua_pushnil(L);
			}
		}
		else
		{
			luaL_argcheck(L, 0, 2, "Index out of range.");
		}
	}

	return(1);
}


int AActorsLen(lua_State * L) 
{
	lua_pushinteger(L, pAActorArray->Count);

	return(1);
}



/******************************************************************************
*
* Lua FNames Functions
*
******************************************************************************/
int FNamesIndex(lua_State * L) 
{
	DWORD  Index;
	char   Name[MaxNameSize];
	WCHAR  WName[MaxNameSize];

	if (lua_type(L, 2) == LUA_TSTRING)
	{
		wsprintfW(WName, L"%S", luaL_checkstring(L, 2));
		for (Index = 0; Index < pFNameEntryArray->Count; ++Index)
		{
			if (pFNameEntryArray->pArray[Index])
			{
				if (!wcscmp(WName, pFNameEntryArray->pArray[Index]->Name))
					break;
			}
		}

		if (Index < pFNameEntryArray->Count)
			lua_pushinteger(L, Index);
		else
			lua_pushnil(L);
	}

	else
	{
		Index = luaL_checkinteger(L, 2);
		if (Index < pFNameEntryArray->Count)
		{
			if (pFNameEntryArray->pArray[Index])
			{
				sprintf(Name, "%S", GetFName(Index));
				lua_pushstring(L, Name);
			}
			else
			{
				lua_pushnil(L);
			}
		}
		else
		{
			luaL_argcheck(L, 0, 2, "Index out of range.");
		}
	}

	return(1);
}


int FNamesLen(lua_State * L) 
{
	lua_pushinteger(L, pFNameEntryArray->Count);

	return(1);
}


/******************************************************************************
*
* Lua UFunction Functions
*
******************************************************************************/
int UFunctionCall(lua_State * L) 
{
	WORD            OldNativeIndex;
	UFunction *     pFunction;
	UFunctionData * pFunctionData;
	UObject *       pObject;
	char *          pParam;
	char *          pParamBlock;
	DWORD *         pProcessEvent;
	UProperty *     pProperty;
	int             ParamBlockSize;
	int             ParamIndex;
	int             Results;

	Results = 0;
	try
	{
		pFunctionData = (UFunctionData *)luaL_checkudata(L, 1, EngineUFunction);

		//Validate the parameters.
		ParamIndex = 2;
		ParamBlockSize = 0;
		pProperty  = (UProperty *)pFunctionData->pFunction->pChildren;
		while (pProperty)
		{
			if (pProperty->IsA(pPropertyClass) && (pProperty->Flags & 0x80)) //CPF_Parm
			{
				ParamBlockSize += pProperty->ElementSize;

				if (!(pProperty->Flags & 0x400)) //CPF_ReturnParm
				{
					if (!(pProperty->Flags & 0x10) || (ParamIndex <= lua_gettop(L))) //CPF_OptionalParm
						ValidateParam(L, pProperty, ParamIndex);

					++ParamIndex;
				}
			}

			pProperty = (UProperty *)pProperty->pNext;
		}


		//Fill out the parameter block.
		pParamBlock = new char[ParamBlockSize];
		memset(pParamBlock, 0, ParamBlockSize);

		try
		{
			pParam = pParamBlock;
			ParamIndex = 2;
			pProperty  = (UProperty *)pFunctionData->pFunction->pChildren;
			while (pProperty)
			{
				if (pProperty->IsA(pPropertyClass) && (pProperty->Flags & 0x80)) //CPF_Parm
				{
					if (!(pProperty->Flags & 0x400)) //CPF_ReturnParm
					{
						if (!(pProperty->Flags & 0x10) || (ParamIndex <= lua_gettop(L))) //CPF_OptionalParm
							InParam(L, pProperty, ParamIndex, pParam);

						++ParamIndex;
					}

					pParam += pProperty->ElementSize;
				}

				pProperty = (UProperty *)pProperty->pNext;
			}

			//Log("ID: %d", pFunctionData->pFunction->NativeIndex);

			//Call the function.
			OldNativeIndex = pFunctionData->pFunction->NativeIndex;
			pFunctionData->pFunction->NativeIndex = 0;

			 //pFunctionData->pFunction->Flags |= ~0x400;

			pObject = pFunctionData->pObject;
			pFunction = pFunctionData->pFunction;
			pProcessEvent = (DWORD*) processEvent;
        		//pObject->pVMT[4];

			__asm xor  eax,eax;
			__asm push eax;
			__asm mov  eax,pParamBlock;
			__asm push eax;
			__asm mov  eax,pFunction;
			__asm push eax;
			__asm mov  ecx,pObject;
			__asm call pProcessEvent;

			//pFunctionData->pFunction->Flags|= 0x400;
			pFunctionData->pFunction->NativeIndex = OldNativeIndex;


			//Push the return value.
			pParam = pParamBlock;
			pProperty  = (UProperty *)pFunctionData->pFunction->pChildren;
			while (pProperty)
			{
				if (pProperty->IsA(pPropertyClass) && (pProperty->Flags & 0x80)) //CPF_Parm
				{
					if (pProperty->Flags & 0x400) //CPF_ReturnParm
					{
						++Results;
						CreateReturn(L, pProperty, pParam);
					}

					pParam += pProperty->ElementSize;
				}

				pProperty = (UProperty *)pProperty->pNext;
			}


			//Push the out parameters, in order
			pParam = pParamBlock;
			pProperty  = (UProperty *)pFunctionData->pFunction->pChildren;
			while (pProperty)
			{
				if (pProperty->IsA(pPropertyClass) && (pProperty->Flags & 0x80)) //CPF_Parm
				{
					if (pProperty->Flags & 0x400) //CPF_ReturnParm
					{
					}

					else if (pProperty->Flags & 0x100) //CPF_OutParm
					{
						++Results;
						CreateReturn(L, pProperty, pParam);
					}
          //Since FMalloc.Realloc is complex, use own destructor when it was an inparam
  				if (pProperty->IsA(pStrPropertyClass) && !(pProperty->Flags & 0x10) && !(pProperty->Flags & 0x400)) //CPF_OptionalParm
						FStringDestructor(pParam);

					pParam += pProperty->ElementSize;
				}

				pProperty = (UProperty *)pProperty->pNext;
			}
		}
		catch (...)
		{
			luaL_argcheck(L, 0, 1, "GPF in Call2");
		}

		delete[] pParamBlock;
	}
	catch(...)
	{
		luaL_argcheck(L, 0, 1, "GPF in Call");
		ResetPropertyCache();
	}

	return(Results);
}


int UFunctionIndex(lua_State * L) 
{
	DWORD           Index;
	UFunctionData * pFunctionData;


	pFunctionData = (UFunctionData *)luaL_checkudata(L, 1, EngineUFunction);
	Index = luaL_checkinteger(L, 2);
	if (Index < pFunctionData->pFunction->Script.Count)
		lua_pushinteger(L, pFunctionData->pFunction->Script.pArray[Index]);
	else
		luaL_argcheck(L, 0, 2, "Index out of range.");

	return(1);
}


int UFunctionNewIndex(lua_State * L) 
{
	DWORD           Index;
	UFunctionData * pFunctionData;
	DWORD           Value;


	pFunctionData = (UFunctionData *)luaL_checkudata(L, 1, EngineUFunction);
	Index = luaL_checkinteger(L, 2);
	Value = luaL_checkinteger(L, 3);
	if (Index < pFunctionData->pFunction->Script.Count)
		pFunctionData->pFunction->Script.pArray[Index] = (BYTE)Value;
	else
		luaL_argcheck(L, 0, 2, "Index out of range.");

	return(0);
}


int UFunctionLen(lua_State * L) 
{
	UFunctionData * pFunctionData;


	pFunctionData = (UFunctionData *)luaL_checkudata(L, 1, EngineUFunction);
	lua_pushinteger(L, pFunctionData->pFunction->Script.Count);

	return(1);
}


/******************************************************************************
*
* Lua UObject Functions
*
******************************************************************************/
int UObjectIndex(lua_State * L) 
{
	UProperty *   pProperty;
	const char *  pPropertyName;
	UObjectData * pObjectData;


	pObjectData = (UObjectData *)luaL_checkudata(L, 1, EngineUObject);
	pPropertyName = lua_tostring(L, 2);

	pProperty = FindProperty(L, pObjectData->pClass, pPropertyName);
	GetProperty(L, pObjectData->pObject, pProperty, pObjectData->pData, NoIndex);

	return(1);
}


int UObjectNewIndex(lua_State * L) 
{
	UObjectData * pObjectData;
	UProperty *   pProperty;
	const char *  pPropertyName;


	pObjectData = (UObjectData *)luaL_checkudata(L, 1, EngineUObject);
	pPropertyName = lua_tostring(L, 2);

	pProperty = FindProperty(L, pObjectData->pClass, pPropertyName);
	SetProperty(L, pProperty, pObjectData->pData, NoIndex);

	return(0);
}


int UObjectLen(lua_State * L) 
{
	UObjectData * pObjectData;

	pObjectData = (UObjectData *)luaL_checkudata(L, 1, EngineUObject);
	if ((char *)pObjectData->pObject != pObjectData->pData)
		luaL_argcheck(L, 0, 1, "Expecting a UObject");

	lua_pushinteger(L, pObjectData->pObject->ObjectInternal);

	return(1);
}


/******************************************************************************
*
* Lua UObjects Functions
*
******************************************************************************/
int UObjectsIndex(lua_State * L) 
{
	DWORD           Index;
	UFunctionData * pFunctionData;
	UObject *       pObject;
	UObjectData *   pObjectData;


	pObject = 0;
	if (lua_type(L, 2) == LUA_TSTRING)
	{
		pObject = FindObjectByFullName(luaL_checkstring(L, 2));
	}

	else
	{
		Index = luaL_checkinteger(L, 2);
		if (Index < pUObjectArray->Count)
			pObject = pUObjectArray->pArray[Index];
		else
			luaL_argcheck(L, 0, 2, "Index out of range.");
	}

	if (pObject)
	{
		if (pObject->IsA(pFunctionClass))
		{
			pFunctionData = (UFunctionData *)lua_newuserdata(L, sizeof(*pFunctionData));
			pFunctionData->pObject   = pObject;
			pFunctionData->pFunction = (UFunction *)pObject;

			luaL_getmetatable(L, EngineUFunction);
			lua_setmetatable(L, -2);
		}

		else
		{
			pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
			pObjectData->pObject    = pObject;
			pObjectData->pClass     = pObject->pClass;
			pObjectData->pData      = (char *)pObject;
			pObjectData->ActorIndex = (DWORD)-1;

			luaL_getmetatable(L, EngineUObject);
			lua_setmetatable(L, -2);
		}
	}
	else
	{
		lua_pushnil(L);
	}

	return(1);
}


int UObjectsLen(lua_State * L) 
{
	lua_pushinteger(L, pUObjectArray->Count);

	return(1);
}


/******************************************************************************
*
* Lua UProperty Functions
*
******************************************************************************/
int UPropertyIndex(lua_State * L) 
{
	int           Index;
	UObjectProp * pObjectProp;
	UProperty *   pProperty;
	const char *  pPropertyName;



	pObjectProp = (UObjectProp *)luaL_checkudata(L, 1, EngineUProperty);

	if (lua_type(L, 2) == LUA_TSTRING)
	{
		pPropertyName = lua_tostring(L, 2);

		pProperty = FindProperty(L, pObjectProp->pProperty->pRelatedClass, 
			pPropertyName);
		GetProperty(L, pObjectProp->pObject, pProperty, 
			pObjectProp->pData + pObjectProp->pProperty->CStructOffset, NoIndex);
	}

	else
	{
		Index = luaL_checkinteger(L, 2);
		GetProperty(L, pObjectProp->pObject, pObjectProp->pProperty, 
			pObjectProp->pData, Index);
	}

	return(1);
}


int UPropertyNewIndex(lua_State * L) 
{
	int           Index;
	UObjectProp * pObjectProp;
	UProperty *   pProperty;
	const char *  pPropertyName;



	pObjectProp = (UObjectProp *)luaL_checkudata(L, 1, EngineUProperty);

	if (lua_type(L, 2) == LUA_TSTRING)
	{
		pPropertyName = lua_tostring(L, 2);

		pProperty = FindProperty(L, pObjectProp->pProperty->pRelatedClass, 
			pPropertyName);
		SetProperty(L, pProperty, pObjectProp->pData + 
			pObjectProp->pProperty->CStructOffset, NoIndex);
	}

	else
	{
		Index = luaL_checkinteger(L, 2);
		SetProperty(L, pObjectProp->pProperty, pObjectProp->pData, Index);
	}

	return(0);
}

int UPropertyLen(lua_State * L) 
{
	UObjectProp * pObjectProp = (UObjectProp *)luaL_checkudata(L, 1, EngineUProperty);

  if (pObjectProp->pProperty->IsA(pArrayPropertyClass)) {
    lua_pushinteger(L, ((TArray<void> *)(pObjectProp->pData + pObjectProp->pProperty->CStructOffset))->Count);
  } else {
    lua_pushinteger(L, pObjectProp->pProperty->ElementCount);
  }

	return(1);
}

/******************************************************************************
*
* Lua Engine Library
*
******************************************************************************/
lua_State* L;


const struct luaL_reg EngineLib[] = 
{
	{"CloseConsole", CloseConsole},
	{"Dump", Dump},
	{"DumpClass", DumpClass},
	{"FindFirst", FindFirst},
	{"FindFirstAActor", FindFirstAActor},
	{"FindNext", FindNext},
	{"FindNextAActor", FindNextAActor},
	{"FullName", FullName},
	{"IsA", IsA},
	{"Load", Load},
	{"Log", LogString},
	{"New", New},
	{"OpenConsole", OpenConsole},
	{"Restart", Restart},
	{"GetKeyState", GetKeyState},
	{NULL, NULL}
};


int lua_openEngine(lua_State *L) 
{
	luaL_openlib(L, "Engine", EngineLib, 0);

	lua_pushstring(L, "AActors");
	lua_newuserdata(L, 1);
	luaL_newmetatable(L, "Engine.AActors");
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, AActorsIndex);
	lua_settable(L, -3);
	lua_pushstring(L, "__len");
	lua_pushcfunction(L, AActorsLen);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);
	lua_settable(L, -3);

	lua_pushstring(L, "FNames");
	lua_newuserdata(L, 1);
	luaL_newmetatable(L, "Engine.FNames");
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, FNamesIndex);
	lua_settable(L, -3);
	lua_pushstring(L, "__len");
	lua_pushcfunction(L, FNamesLen);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);
	lua_settable(L, -3);

	lua_pushstring(L, "UObjects");
	lua_newuserdata(L, 1);
	luaL_newmetatable(L, "Engine.UObjects");
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, UObjectsIndex);
	lua_settable(L, -3);
	lua_pushstring(L, "__len");
	lua_pushcfunction(L, UObjectsLen);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);
	lua_settable(L, -3);

	luaL_newmetatable(L, EngineUFunction);
	lua_pushstring(L, "__call");
	lua_pushcfunction(L, UFunctionCall);
	lua_settable(L, -3);
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, UFunctionIndex);
	lua_settable(L, -3);
	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, UFunctionNewIndex);
	lua_settable(L, -3);
	lua_pushstring(L, "__len");
	lua_pushcfunction(L, UFunctionLen);
	lua_settable(L, -3);
	lua_pop(L, 1);

	luaL_newmetatable(L, EngineUObject);
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, UObjectIndex);
	lua_settable(L, -3);
	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, UObjectNewIndex);
	lua_settable(L, -3);
	lua_pushstring(L, "__len");
	lua_pushcfunction(L, UObjectLen);
	lua_settable(L, -3);
	lua_pop(L, 1);

	luaL_newmetatable(L, EngineUProperty);
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, UPropertyIndex);
	lua_settable(L, -3);
	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, UPropertyNewIndex);
	lua_settable(L, -3);
	lua_pushstring(L, "__len");
	lua_pushcfunction(L, UPropertyLen);
	lua_settable(L, -3);
	lua_pop(L, 1);

	return(1);
}


void ShutdownLua(void)
{
	if (L)
	{
		Log("System: Lua Shutdown");
		lua_close(L);
		L = 0;
	}
}


void InitLua(void)
{
	if (!L)
	{
		ResetPropertyCache();
		char szBotPath[MAX_PATH];

		Log("System: Lua Started");
		fputs("Lua Restarted\n", stdout);

		if(hLogFile != NULL && bLogActive) //added
		{
			fclose(hLogFile);
			hLogFile = fopen(szLogFile, "a+");
		}

		//Log("xxxx");

		L = lua_open();

		//Log("L");

		luaL_openlibs(L);

		//Log("luaL_openlibs");

		lua_openEngine(L);

		//Log("lua_openEngine");

		lua_gc(L, LUA_GCRESTART, 0);

		//Log("lua_gc");

		lua_pushcfunction(L, Load);
		
		//Log("xdsddaxx");

		sprintf(szBotPath, "%s%s", Path, DEFAULT_BOTFILE);
		lua_pushstring(L, DEFAULT_BOTFILE);
		lua_pcall(L, 1, 0, 0);
		
		fputs("\n> ", stdout);
	}
	//else
		//Log("L NULLFDFDLf");
}


/******************************************************************************
*
* Lua Callbacks.
*
******************************************************************************/

int KeyEvent(DWORD Key, DWORD Action)
{
	int Result;

	Result = 0;
	try
	{
		if (L)
		{
			lua_settop(L, 0);
			lua_getglobal(L, "KeyEvent");
			if (!lua_isnil(L, -1))
			{
				lua_pushinteger(L, Key);
				lua_pushinteger(L, Action);
				//lua_pushnumber(L, Value);

				if (lua_pcall(L, 2, 1, 0))
				{
					const char * pError = lua_tostring(L, -1);
					if (pError)
						Log("KeyEvent Error: %s", pError);
					else
						Log("KeyEvent Error: [unknown]");
				}

				if (!lua_isboolean(L, -1))
				{
					Log("KeyEvent Error: KeyEvent needs to return a boolean.");
				}
				else
				{
					Result = lua_toboolean(L, -1);
				}
			}
		}
	}
	catch(...)
	{
		Log("KeyEvent Error: GPF");
		ResetPropertyCache();
	}

	return(Result);
}


void PostRender(UObject * pCanvas)
{
	UObjectData * pObjectData;


	try
	{
		if (L)
		{
			lua_settop(L, 0);
			lua_getglobal(L, "PostRender");
			if (!lua_isnil(L, -1))
			{
				if (pCanvas)
				{
					pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
					pObjectData->pObject    = pCanvas;
					pObjectData->pClass     = pCanvas->pClass;
					pObjectData->pData      = (char *)pCanvas;
					pObjectData->ActorIndex = (DWORD)-1;

					luaL_getmetatable(L, EngineUObject);
					lua_setmetatable(L, -2);
				}
				else
				{
					lua_pushnil(L);
				}

				if (lua_pcall(L, 1, 0, 0))
				{
					const char * pError = lua_tostring(L, -1);
					if (pError)
						Log("PostRender Error: %s", pError);
					else
						Log("PostRender Error: [unknown]");
				}
			}
		}
	}
	catch(...)
	{
		Log("PostRender Error: GPF");
		ResetPropertyCache();
	}

}


/*void PreRender(UObject * pCanvas)
{
	UObjectData * pObjectData;

	try
	{
		if (L)
		{
			lua_settop(L, 0);
			lua_getglobal(L, "PreRender");
			if (!lua_isnil(L, -1))
			{
				if (pCanvas)
				{
					pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
					pObjectData->pObject    = pCanvas;
					pObjectData->pClass     = pCanvas->pClass;
					pObjectData->pData      = (char *)pCanvas;
					pObjectData->ActorIndex = (DWORD)-1;

					luaL_getmetatable(L, EngineUObject);
					lua_setmetatable(L, -2);
				}
				else
				{
					lua_pushnil(L);
				}

				if (lua_pcall(L, 1, 0, 0))
				{
					const char * pError = lua_tostring(L, -1);
					if (pError)
						Log("PreRender Error: %s", pError);
					else
						Log("PreRender Error: [unknown]");
				}
			}
		}
	}
	catch(...)
	{
		Log("PreRender Error: GPF");
		ResetPropertyCache();
	}
}*/


void Tick(float DeltaTime)
{
	UObjectData * pObjectData;

	//Reload Lua Script

	if (RestartLua)
	{
		RestartLua = 0;
		ShutdownLua();
	}

	InitLua();

	char szWindowText[50];
	HWND fgw;

	if((fgw = GetForegroundWindow()) != NULL)
	{

		GetWindowTextA(fgw, szWindowText, sizeof(szWindowText));

    		if(strstr(szWindowText, WINDOWTEXT))
		{
			if ((GetAsyncKeyState(BASEKEY) & 0x8000) &&
				(GetAsyncKeyState((BYTE)VkKeyScan(RESTART_KEY)) & 0x8000))
			{
				RestartLua = true;
				Sleep(250);
			}

			if ((GetAsyncKeyState(BASEKEY) & 0x8000) &&
				(GetAsyncKeyState((BYTE)VkKeyScan(OPEN_LOG_KEY)) & 0x8000) &&
				bLogActive)
			{
				ShellExecuteA(0, "open", "notepad.exe", szLogFile, "", 1);
				Sleep(250);
			}

			//Toggle the console
			static int ConsoleOneShot;
			if ((GetAsyncKeyState(BASEKEY) & 0x8000) &&
				(GetAsyncKeyState((BYTE)VkKeyScan(OPEN_LUA_CONSOLE)) & 0x8000))
			{
				if (!ConsoleOneShot)
				{
					ConsoleOneShot = 1;
					if (hConsole)
						CloseConsole(L);
					else
						OpenConsole(L);
				}
			}
			else
			{
				ConsoleOneShot = 0;
			}
		}
	}

	//Process any console data.
	if (hConsole && Input[0])
	{
		try
		{
			if (L)
			{
				lua_settop(L, 0);
				if (luaL_dostring(L, &Input[1]))
				{
					const char * pError = lua_tostring(L, -1);

					fputs("Console Error: ", stdout);
					if (pError)
					{
						fputs(pError, stdout);
						Log("Console Error: %s", pError);
					}
					else
					{
						fputs("[unknown]", stdout);
						Log("Console Error: [unknown]");
					}

					fputs("\n\n", stdout);
				}

				fputs("> ", stdout);

				Input[0] = 0;
				SetEvent(hInputEvent);
			}
		}
		catch(...)
		{
			Log("Console Error: GPF");
			ResetPropertyCache();
		}
	}

	//Call Lua Tick

	try
	{
		if (L)
		{
			lua_settop(L, 0);
			lua_getglobal(L, "Tick");
			if (!lua_isnil(L, -1))
			{
				lua_pushnumber(L, DeltaTime);

				if (pViewport)
				{
					pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
					pObjectData->pObject    = pViewport;
					pObjectData->pClass     = pViewport->pClass;
					pObjectData->pData      = (char *)pViewport;
					pObjectData->ActorIndex = (DWORD)-1;

					luaL_getmetatable(L, EngineUObject);
					lua_setmetatable(L, -2);
				}
				else
				{
					lua_pushnil(L);
				}

				if (lua_pcall(L, 2, 0, 0))
				{
					const char * pError = lua_tostring(L, -1);
					if (pError)
						Log("Tick Error: %s", pError);
					else
						Log("Tick Error: [unknown]");
				}
			}
		}
	}
	catch(...)
	{
		Log("Tick Error: GPF");
		ResetPropertyCache();
	}
}

/*void PreTick(float DeltaTime)
{
	UObjectData * pObjectData;

	//Call Lua Tick
	try
	{
		if (L)
		{
			lua_settop(L, 0);
			lua_getglobal(L, "PreTick");
			if (!lua_isnil(L, -1))
			{
				lua_pushnumber(L, DeltaTime);

				if (pViewport)
				{
          if (!pAActorArray) {
            pAActorArray = &(((ULevel*) FindObjectByFullName(LEVEL_NAME))->ActorArray);
          }

					pObjectData = (UObjectData *)lua_newuserdata(L, sizeof(*pObjectData));
					pObjectData->pObject    = pViewport;
					pObjectData->pClass     = pViewport->pClass;
					pObjectData->pData      = (char *)pViewport;
					pObjectData->ActorIndex = (DWORD)-1;

					luaL_getmetatable(L, EngineUObject);
					lua_setmetatable(L, -2);
				}
				else
				{
					lua_pushnil(L);
				}

				if (lua_pcall(L, 2, 0, 0))
				{
					const char * pError = lua_tostring(L, -1);
					if (pError)
						Log("PreTick Error: %s", pError);
					else
						Log("PreTick Error: [unknown]");
				}
			}
		}
	}
	catch(...)
	{
		Log("PreTick Error: GPF");
		ResetPropertyCache();
	}
}*/

void PostBeginPlay()
{

	try
	{
		if (L)
		{
			lua_settop(L, 0);
			lua_getglobal(L, "PostBeginPlay");
			if (!lua_isnil(L, -1))
			{
  			lua_pushnil(L);
	
				if (lua_pcall(L, 1, 0, 0))
				{
					const char * pError = lua_tostring(L, -1);
					if (pError)
						Log("PostBeginPlay Error: %s", pError);
					else
						Log("PostBeginPlay Error: [unknown]");
				}
			}
		}
	}
	catch(...)
	{
		Log("PostBeginPlay Error: GPF");
		ResetPropertyCache();
	}
}


/******************************************************************************
*
* DLL Attach/Detach
*
******************************************************************************/
void ProcessAttach(HINSTANCE hInstance)
{
	char FileName[MAX_PATH];
	char* pChar;

	//get base path
	GetModuleFileNameA(hInstance, FileName, sizeof(FileName));

	//Strip our file from the path.
	strcpy(Path, FileName);
	pChar = strrchr(Path, '\\');
	if (pChar)
		pChar[1] = 0;

	//Start a log file.
	if(strlen(DEFAULT_LOGFILE) > 0)
	{
		sprintf(szLogFile, "%s%s", Path, DEFAULT_LOGFILE);
		hLogFile = fopen(szLogFile, "w");
		bLogActive = true;
		
		Log("System: LScript Started");
	}

	//Create the event.
	hInputEvent = CreateEvent(0, 0, 0, 0);
}

void ProcessDetach()
{
	CloseConsole(0);
	CloseHandle(hInputEvent);
	ShutdownLua();

	Log("System: LScript Shutdown");
	
	if(hLogFile)
		fclose(hLogFile);
}


/******************************************************************************
*******************************************************************************
*******************************************************************************
* 
* Hook code
*
*******************************************************************************
*******************************************************************************
******************************************************************************/
#define MODULE_NAME "UT3.exe"

struct UFunction *Function = NULL;
void * Parms;
void * Result;

typedef void (__stdcall *tProcessEvent ) ( UFunction*, void*, void* );
tProcessEvent pProcessEvent;
tProcessEvent pProcessEvent2;
tProcessEvent pProcessEvent2x;
tProcessEvent pProcessEvent3;

DWORD sEAX;
#define ProcessEventOffset 0x00BD3F70 - 0x00400000
DWORD ProcessEventReturn;

UObject* Obj = NULL;
UObject* pCallObject2 = NULL;
char FunctionName[256];
char FunctionName2[256];
char FunctionName3[256];

struct UGameViewportClient_eventTick_Parms
{
	float                                              DeltaTime;                                         // 0x0000 0x0004            0x00000080
};

struct UUTGameViewportClient_eventPostRender_Parms
{
	struct UObject*                                     Canvas;                                            // 0x0000 0x0004            0x00000080
	int                                                I;                                                 // 0x0004 0x0004           
	struct UObject*                         PC;                                                // 0x0008 0x0004           
	unsigned char                                      OldTransitionType;                                 // 0x000C 0x0001           
	struct UObject*                                AD;                                                // 0x0010 0x0004           
};

struct UUTConsole_InputKey_Parms
{
	int                                                ControllerId;                                      // 0x0000 0x0004            0x00000080
	FName                                              Key;                                               // 0x0004 0x0008            0x00000080
	DWORD												dummy0;
	unsigned char                                      Event;                                             // 0x000C 0x0001            0x00000080
	float                                              AmountDepressed;                                   // 0x0010 0x0004            0x00000090 CPF_OptionalParm
	unsigned long                                      bGamepad;                                          // 0x0014 0x0004 0x00000001 0x00000090 CPF_OptionalParm
	unsigned long                                      ReturnValue;                                       // 0x0018 0x0004 0x00000001 0x00000580 CPF_ReturnParm CPF_OutParm
	struct UObject*										 UTSceneClient;                                     // 0x001C 0x0004           
};

bool bTickcalled = false;

void WINAPI hkProcessEvent (struct UFunction* pFunc, void* PArms, void* Result )
{
    __asm mov pCallObject2, ecx;

	_asm pushad
	
    if ( pFunc != NULL)
    {
		pFunc->GetFullName(FunctionName2);

		if(strcmp(FunctionName2, "Function Engine.WorldInfo.PostBeginPlay"/*L"Function UTGame.UTHUD.PostRender"*/) == 0)
		{
			PostBeginPlay();
		}
    }

    __asm popad

    __asm
    {
        push Result
        push PArms
        push pFunc
        call pProcessEvent
    }

}

void WINAPI hkProcessEvent2 (struct UFunction* pFunc, void* PArms, void* Result )
{
    __asm mov Obj, ecx;

	_asm pushad
	
    if ( pFunc != NULL)
    {
		pFunc->GetFullName(FunctionName);

		if(strcmp(FunctionName, "Function UTGame.UTGameViewportClient.PostRender") == 0)
		{
			if(bTickcalled)
			{
				PostRender( ( ( UUTGameViewportClient_eventPostRender_Parms* )PArms )->Canvas );
			}
			
		}
		else if(strcmp(FunctionName, "Function Engine.GameViewportClient.Tick") == 0)
		{
			Tick(( ( UGameViewportClient_eventTick_Parms* )PArms )->DeltaTime);
			bTickcalled = true;
		}
    }

    __asm popad

    __asm
    {
        push Result
        push PArms
        push pFunc
        call pProcessEvent2
    }

}

void WINAPI hkProcessEvent3 (struct UFunction* pFunc, void* PArms, void* Result )
{
	_asm pushad
	
    if ( pFunc != NULL)
    {
		pFunc->GetFullName(FunctionName3);

		if(strcmp(FunctionName3, "Function UTGame.UTConsole.InputKey"/*L"Function UTGame.UTEntryHUD.PostRender"*/) == 0)
		{
			KeyEvent(((UUTConsole_InputKey_Parms*)PArms)->Key, ((UUTConsole_InputKey_Parms*)PArms)->Event);
			
		}
    }

    __asm popad

    __asm
    {
        push Result
        push PArms
        push pFunc
        call pProcessEvent3
    }


}

unsigned long ModuleThread(bool bWait = false)
{
	EngineInit();

	bTickcalled = false;

	UObject* po = (UObject *)FindObjectByFullName("UTGameViewportClient Transient.GameEngine.UTGameViewportClient"/*"UTEntryHUD TheWorld.PersistentLevel.UTEntryHUD"*/);

	toolkit::VMTHook* hook2 = new toolkit::VMTHook(po);
	pProcessEvent2 = hook2->GetMethod<tProcessEvent>(59);
	hook2->HookMethod(&hkProcessEvent2, 59);

	UObject* pw = (UObject *)FindObjectByFullName("WorldInfo TheWorld.PersistentLevel.WorldInfo"/*"UTEntryHUD TheWorld.PersistentLevel.UTEntryHUD"*/);

	toolkit::VMTHook* hook = new toolkit::VMTHook(pw);
	pProcessEvent = hook->GetMethod<tProcessEvent>(59);
	hook->HookMethod(&hkProcessEvent, 59);

	UObject* pcc = (UObject *)FindObjectByFullName("UTConsole GameEngine.UTGameViewportClient.UTConsole"/*"UTEntryHUD TheWorld.PersistentLevel.UTEntryHUD"*/);

	toolkit::VMTHook* hook3 = new toolkit::VMTHook(pcc);
	pProcessEvent3 = hook3->GetMethod<tProcessEvent>(59);
	hook3->HookMethod(&hkProcessEvent3, 59);

	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{
      switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
		ProcessAttach(hModule);
		CreateThread ( NULL, 0, ( LPTHREAD_START_ROUTINE )ModuleThread, NULL, 0, NULL );
	    }
		break;
		case DLL_PROCESS_DETACH:
		{
  		
			ProcessDetach();
		}
		break;
	}
	return TRUE;

    
}
