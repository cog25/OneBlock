
#include "mcf.h"
#include "console.h"
#include "codewrite.h"
#include "gen/version.h"

#include <KR3/data/map.h>
#include <KR3/data/crypt.h>
#include <KR3/io/selfbufferedstream.h>
#include <KR3/util/pdb.h>
#include <KRWin/hook.h>
#include <KRWin/handle.h>

/*
BedrockLog::log
common::getServerVersionString()
PermissionsFile
WhitelistFile

permissions.json
server.properties
whitelist.json
*/

using namespace kr;
using namespace hook;

#define HASH_1_16_1_02 "26694FF65D20A61D6A3731A2E827AE61"
#define HASH_1_16_0_2 "B3385584CF5F99FEB29180A5A04A0788"
#define HASH_1_16_20_03 "B575B802BE9C4B17F580B2485DE06D98"
#define HASH_1_16_100_04 "6E4E57777403D4359C899F3308436B27"
#define HASH_1_16_200_02 "5C9351D3BB8FCDA6D7037F9911A5F03E"

#define FNNAME(v) renamer.varNameToCppName(#v), v

MinecraftFunctionTable g_mcf;
ServerInstance* g_server;

class Anchor
{
private:
	static constexpr size_t SIZE = 32;
	byte m_backup[SIZE];
	size_t m_size;
	Unprotector m_unpro;

public:
	Anchor(void* junctionPoint) noexcept
		:m_unpro(junctionPoint, SIZE)
	{
		memcpy(m_backup, junctionPoint, SIZE);
		CodeWriter writer(junctionPoint, SIZE);
		static void (*anchor)(Anchor * _this) = [](Anchor * _this){
			memcpy(_this->m_unpro, _this->m_backup, _this->SIZE);
			requestDebugger();
			debug();
		};
		writer.mov(RCX, (uintptr_t)this);
		writer.sub(RSP, 0x28);
		writer.call(anchor, RAX);
		writer.add(RSP, 0x28);
		m_size = (byte*)writer.end() - (byte*)junctionPoint;
	}
};

McftRenamer::McftRenamer() noexcept
{
	m_str2.reserve(32);
	m_str1.reserve(32);
}

View<McftRenamer::Entry> McftRenamer::getEntires() noexcept
{
#define ENTRY(x) {#x, (void* (MinecraftFunctionTable::*))&MinecraftFunctionTable::x, 0}
#define ENTRY_IDX(x, n) {#x, (void* (MinecraftFunctionTable::*))&MinecraftFunctionTable::x, n}
	static const Entry entries[] = {
		ENTRY(NetworkHandler$_sortAndPacketizeEvents),
		ENTRY(MinecraftPackets$createPacket),
		ENTRY(ServerNetworkHandler$_getServerPlayer),
		ENTRY(NetworkHandler$getEncryptedPeerForUser),
		ENTRY(NetworkHandler$_getConnectionFromId),
		ENTRY(ExtendedCertificate$getXuid),
		ENTRY(ExtendedCertificate$getIdentityName),
		ENTRY(ServerInstance$_update),
		ENTRY(Minecraft$update),
		ENTRY_IDX(NetworkHandler$onConnectionClosed, 1),
		ENTRY(ServerInstance$ServerInstance),
		ENTRY(DedicatedServer$start),
		ENTRY(ScriptEngine$startScriptLoading),
		ENTRY(ScriptEngine$isScriptingEnabled),
		ENTRY(std$string$_Tidy_deallocate),
		ENTRY(std$string$assign),
		ENTRY_IDX(std$string$append, 1),
		ENTRY(std$string$resize),
		ENTRY(MinecraftCommands$executeCommand),
		ENTRY(DedicatedServer$stop),
		ENTRY(StopCommand$mServer),
		ENTRY(NetworkHandler$send),
		ENTRY(NetworkIdentifier$getHash),
		ENTRY(NetworkIdentifier$$_equals_),
		ENTRY(Crypto$Random$generateUUID),
		ENTRY(BaseAttributeMap$getMutableInstance),
		ENTRY(Level$createDimension),
		ENTRY(Actor$dtor$Actor),
		ENTRY(Level$fetchEntity),
		ENTRY(NetworkHandler$_sendInternal),
		ENTRY(std$_Allocate$_alloc16_),
		ENTRY(ServerPlayer$$_vftable_),
		ENTRY(ServerPlayer$sendNetworkPacket),
		ENTRY(LoopbackPacketSender$sendToClients),
		ENTRY(Level$removeEntityReferences),
		ENTRY(google_breakpad$ExceptionHandler$HandleException),
		ENTRY(google_breakpad$ExceptionHandler$HandleInvalidParameter),
		ENTRY(BedrockLogOut),
		ENTRY(CommandOutputSender$send),
		ENTRY(std$_LaunchPad$_stdin_t_$_Execute$_0_),
		ENTRY(ScriptEngine$dtor$ScriptEngine),
		ENTRY(ServerInstance$startServerThread),
		ENTRY(main),
		ENTRY(__scrt_common_main_seh),
		ENTRY($_game_thread_lambda_$$_call_),
		ENTRY($_game_thread_start_t_),
		ENTRY(std$_Pad$_Release),
		ENTRY(ScriptEngine$initialize),
		ENTRY(Actor$getUniqueID),
		ENTRY(PacketViolationHandler$_handleViolation),
		ENTRY(MinecraftServerScriptEngine$onServerThreadStarted),
	};
#undef ENTRY

	return entries;
}
View<Text> McftRenamer::getReplaceMap() noexcept
{
	static const Text list[] = {
		"basic_string<char,std::char_traits<char>,std::allocator<char> >"_tx, "string"_tx,
		"::"_tx, "$"_tx,
		"operator=="_tx, "$_equals_"_tx,
		"operator()"_tx, "$_call_"_tx,
		"<16,std$_Default_allocate_traits,0>"_tx, "$_alloc16_"_tx,
		"<0>"_tx, "$_0_"_tx,
		"`vftable'"_tx, "$_vftable_"_tx,
		"<lambda_85c8d3d148027f864c62d97cac0c7e52>"_tx, "$_game_thread_lambda_"_tx,
		"<lambda_cab8a9f6b80f4de6ca3785c051efa45e>"_tx, "$_std_input_lambda_"_tx,
		"<std$unique_ptr<std$tuple<$_std_input_lambda_ >,std$default_delete<std$tuple<$_std_input_lambda_ > > > >"_tx, "$_stdin_t_"_tx,
		"~"_tx, "dtor$"_tx,
		"std::_LaunchPad<std::unique_ptr<std::tuple<$_game_thread_lambda_ >,std::default_delete<std::tuple<$_game_thread_lambda_ > > > >::_Go"_tx, "$_game_thread_start_t_"_tx,
	};
	return list;
}
	
Text McftRenamer::cppNameToVarName(Text text) noexcept
{
	AText *dst = &m_str2, *src = &m_str1;

	View<Text> rmap = getReplaceMap();
	{
		Text from = *rmap++;
		Text to = *rmap++;
		src->clear();
		text.replace(src, from, to);
	}
	while (!rmap.empty())
	{
		Text from = *rmap++;
		Text to = *rmap++;
		dst->clear();
		src->replace(dst, from, to);

		AText* t = dst;
		dst = src;
		src = t;
	}
	return *src;
}
Text McftRenamer::varNameToCppName(Text text) noexcept
{
	AText* dst = &m_str2, * src = &m_str1;

	View<Text> rmap = getReplaceMap();
	{
		Text from = rmap.readBack();
		Text to = rmap.readBack();
		src->clear();
		text.replace(src, from, to);
	}
	while (!rmap.empty())
	{
		Text from = rmap.readBack();
		Text to = rmap.readBack();
		dst->clear();
		src->replace(dst, from, to);

		AText* t = dst;
		dst = src;
		src = t;
	}
	return *src;
}

namespace
{
	File* openPredefinedFile(Text hash) throws(Error)
	{
		TText16 bdsxPath = win::Module::byName(u"bdsx.dll")->fileName();
		bdsxPath.cut_self(bdsxPath.find_r('\\') + 1);
		bdsxPath << u"predefined";
		File::createDirectory(bdsxPath.c_str());
		bdsxPath << u'\\' << (Utf8ToUtf16)hash << u".ini";
		return File::openRW(bdsxPath.c_str());
	}
}

struct AddressReader
{
	struct FunctionTarget
	{
		Text varName;
		void** dest;
		size_t skipCount;

		FunctionTarget() = default;

		template <typename T>
		FunctionTarget(Text varName, T* ptr, size_t skipCount = 0) noexcept
			:varName(varName), dest((void**)ptr), skipCount(skipCount)
		{
		}
	};

	Must<File> m_predefinedFile;
	Map<Text, FunctionTarget> m_targets;
	MinecraftFunctionTable* const m_table;

public:

	AddressReader(File* file, MinecraftFunctionTable* table, Text hash) noexcept
		:m_table(table), m_predefinedFile(file)
	{
		McftRenamer renamer;
		for (const McftRenamer::Entry& entry : McftRenamer::getEntires())
		{
			m_targets[renamer.varNameToCppName(entry.name)] = { entry.name, &(table->*entry.target), entry.idx };
		}
	}

	void loadFromPredefined() noexcept
	{
		ModuleInfo ptr;
		try
		{
			io::FIStream<char, false> fis = (File*)m_predefinedFile;
			for (;;)
			{
				Text line = fis.readLine();
				
				pcstr equal = line.find_r('=');
				if (equal == nullptr) continue;

				Text name = line.cut(equal).trim();
				Text value = line.subarr(equal+1).trim();

				uintptr_t offset;
				if (value.startsWith("0x"))
				{
					offset = value.subarr(2).to_uintp(16);
				}
				else
				{
					offset = value.to_uintp();
				}

				auto iter = m_targets.find(name);
				if (iter == m_targets.end())
				{
					console.logA(TSZ() << "Unknown function: " << name << '\n');
				}
				else
				{
					*iter->second.dest = ptr(offset);
					m_targets.erase(iter);
				}
			}
		}
		catch (EofException&)
		{
		}
	}
	void loadFromPdb() noexcept
	{
		io::FOStream<char, true, false> fos = (File*)m_predefinedFile;
		
		// remove already existed
		{
			auto iter = m_targets.begin();
			auto end = m_targets.end();
			while (iter != end)
			{
				if (*iter->second.dest != nullptr)
				{
					iter = m_targets.erase(iter);
				}
				else
				{
					iter++;
				}
			}
		}

		try
		{
			console.logA("PdbReader: Load Symbols...\n");
			PdbReader reader;
			reader.load();
			reader.search(nullptr, [&](Text name, void* address, uint32_t typeId) {
				auto iter = m_targets.find(name);
				if (iter == m_targets.end())
				{
					return true;
				}
				if (iter->second.skipCount)
				{
					iter->second.skipCount--;
					return true;
				}
				*iter->second.dest = address;

				TText line = TText::concat(name, " = 0x", hexf((byte*)address - (byte*)reader.base()));
				fos << line << endl;
				line << '\n';
				console.logA(line);
				m_targets.erase(iter);
				return !m_targets.empty();
				});

			if (!m_targets.empty())
			{
				Console::ColorScope _color = FOREGROUND_RED | FOREGROUND_INTENSITY;
				for (auto& item : m_targets)
				{
					Text name = item.first;

					console.logA(TSZ() << name << " not found\n");
				}
			}
		}
		catch (FunctionError& err)
		{
			TSZ tsz;
			tsz << err.getFunctionName() << ": failed, err=";
			err.getMessageTo(&tsz);
			tsz << '\n';
			console.logAnsi(tsz);
		}
	}
};

void MinecraftFunctionTable::load() noexcept
{
	BText<32> hash;
	try
	{
		TText16 moduleName = CurrentApplicationPath();
		hash = (encoder::Hex)(TBuffer)encoder::Md5::hash(File::open(moduleName.data()));
		console.logA(TSZ() << "[BDSX] bedrock_server.exe MD5 = " << hash << '\n');
	}
	catch (Error&)
	{
		console.logA("[BDSX] Cannot read bedrock_server.exe\n");
	}

	try
	{
		File* file = openPredefinedFile(hash);
		int openerr = GetLastError();
		AddressReader reader(file, this, hash);
		if (openerr == ERROR_ALREADY_EXISTS)
		{
			reader.loadFromPredefined();
		}
		else
		{
			console.logA("[BDSX] Predefined does not founded\n");
		}

		if (hash == HASH_1_16_200_02)
		{
			console.logA("[BDSX] MD5 Hash matched(Version == " BDS_VERSION ")\n");
		}
		else
		{
			Console::ColorScope _color = FOREGROUND_RED | FOREGROUND_INTENSITY;
			console.log("[BDSX] MD5 Hash does not Matched(Version != " BDS_VERSION ")\n");
		}
		if (isNotFullLoaded())
		{
			reader.loadFromPdb();
		}
	}
	catch (Error&)
	{
		console.logA("Cannot open the predefined file\n");
	}
}
bool MinecraftFunctionTable::isNotFullLoaded() noexcept
{
	void** iter = (void**)(this);
	void** end = (void**)(this + 1);
	for (; iter != end; iter++)
	{
		if (*iter == nullptr)
		{
			return true;
		}
	}
	return false;
}
void MinecraftFunctionTable::stopServer() noexcept
{
	g_mcf.DedicatedServer$stop((byte*)g_server->server() + 8);
}

void MinecraftFunctionTable::hookOnGameThreadCall(void(*thread)(void* pad, void* lambda)) noexcept
{
	static const byte ORIGINAL_CODE[] = {
		0xE8, 0xff, 0xff, 0xff, 0xff,	// call <bedrock_server.public: void __cdecl std::_Pad::_Release(void) __ptr64>
		0x48, 0x8B, 0xCB,				// mov rcx,rbx
		0xE8, 0xff, 0xff, 0xff, 0xff	// call <bedrock_server.<lambda_2d44924e0d4b2bffcf26349a59ba19df>::operator()>
	};
	Code junction(64);
	junction.mov(RDX, RBX);
	junction.sub(RSP, 0x28);
	junction.call(thread, RAX);
	junction.add(RSP, 0x28);
	junction.ret();

	McftRenamer renamer;
	junction.patchTo(FNNAME($_game_thread_start_t_), 0x1b, ORIGINAL_CODE, RAX, false, { {1, 5}, {9, 13} });
}
void MinecraftFunctionTable::hookOnProgramMainCall(int(*onMain)(int argn, char** argv)) noexcept
{
	static const byte ORIGINAL_CODE[] = {
		0x4C, 0x8B, 0xC0,                // mov r8,rax
		0x48, 0x8B, 0xD7,                // mov rdx,rdi
		0x8B, 0x0B,                      // mov ecx,dword ptr ds:[rbx]
		0xE8, 0xFF, 0xFF, 0xFF, 0xFF,    // call <bedrock_server.main>
		0x8B, 0xD8,                      // mov ebx,eax
	};
	Code junction(64);
	junction.mov(R8, RAX);
	junction.mov(RDX, RDI);
	junction.mov(RCX, DwordPtr, RBX);
	junction.sub(RSP, 0x28);
	junction.call(onMain, RAX);
	junction.add(RSP, 0x28);
	junction.mov(RBX, RAX);
	junction.ret();
	McftRenamer renamer;
	junction.patchTo(FNNAME(__scrt_common_main_seh), 0xFF, ORIGINAL_CODE, RCX, false, { {9, 13} });
}
void MinecraftFunctionTable::hookOnUpdate(void(*update)(Minecraft* mc)) noexcept
{
	/*
	call <bedrock_server.public: bool __cdecl Minecraft::update(void) __ptr64>
	mov eax,dword ptr ds:[r14+F8]
	cmp eax,2
	*/
	static const byte ORIGINAL_CODE[] = {
		0xE8, 0x38, 0xEB, 0x6E, 0x00, 
		0x41, 0x8B, 0x86, 0x08, 0x01, 0x00, 0x00, 
		0x83, 0xF8, 0x02
	};
	Code junction(64);
	junction.sub(RSP, 0x28);
	junction.call(update, RAX);
	junction.add(RSP, 0x28);
	junction.mov(RAX, DwordPtr, R14, 0x108);
	junction.cmp(RAX, 2);
	junction.ret();

	McftRenamer renamer;
	junction.patchTo(FNNAME(ServerInstance$_update), 0x116
		, ORIGINAL_CODE, RAX, false, { {1, 5} });
};
void MinecraftFunctionTable::hookOnPacketRaw(SharedPtr<Packet>* (*onPacket)(OnPacketRBP* rbp, MinecraftPacketIds id, NetworkHandler::Connection* conn)) noexcept
{
	/*
	mov edx,r15d
	lea rcx,qword ptr ss:[rbp+148]
	call <bedrock_server.public: static class std::shared_ptr<class Packet> __cdecl MinecraftPackets::createPacket(enum MinecraftPacketIds)>
	*/
	static const byte ORIGINAL_CODE[] = {
		0x41, 0x8B, 0xD7, 0x48, 0x8D, 0x8D, 0x48, 0x01, 0x00, 0x00, 0xE8, 0xB8, 0x3A, 0x00, 0x00
	};
	Code junction(64);
	junction.sub(RSP, 0x28);
	junction.mov(RCX, RBP); // rbp
	junction.mov(RDX, R15); // packetId
	junction.mov(R8, R13); // Connection
	junction.call(onPacket, RAX);
	junction.add(RSP, 0x28);
	junction.ret();

	McftRenamer renamer;
	junction.patchTo(FNNAME(NetworkHandler$_sortAndPacketizeEvents), 0x2c9,
		ORIGINAL_CODE, RAX, false, { {11, 15}, });
};
void MinecraftFunctionTable::hookOnPacketBefore(ExtendedStreamReadResult* (*onPacketRead)(ExtendedStreamReadResult*, OnPacketRBP*, MinecraftPacketIds)) noexcept
{
	/*
	mov rax,qword ptr ds:[rcx]
	lea r8,qword ptr ss:[rbp+1E0]
	lea rdx,qword ptr ss:[rbp+70]
	call qword ptr ds:[rax+28]
	*/
	static const byte ORIGINAL_CODE[] = {
		0x48, 0x8B, 0x01, 0x4C, 0x8D, 0x85, 0xE0, 0x01, 0x00, 0x00, 0x48, 0x8D, 0x55, 0x70, 0xFF, 0x50, 0x28
	};
	Code junction(64);
	junction.sub(RSP, 0x28);
	junction.write(ORIGINAL_CODE);
	junction.mov(RCX, RAX); // read result
	junction.mov(RDX, RBP); // rbp
	junction.mov(R8, R15); // packetId
	junction.call(onPacketRead, RAX);
	junction.add(RSP, 0x28);
	junction.ret();

	McftRenamer renamer;
	junction.patchTo(FNNAME(NetworkHandler$_sortAndPacketizeEvents), 0x430
		, ORIGINAL_CODE, RAX, false);
};
void MinecraftFunctionTable::hookOnPacketAfter(void(*onPacketAfter)(OnPacketRBP*, MinecraftPacketIds)) noexcept
{
	/*
	mov rax,qword ptr ds:[rcx]
	lea r9,qword ptr ss:[rbp+148]
	mov r8,rsi
	mov rdx,r13
	call qword ptr ds:[rax+8]
	*/
	static const byte ORIGINAL_CODE[] = {
		0x48, 0x8B, 0x01, 0x4C, 0x8D, 0x8D, 0x48, 0x01, 0x00, 0x00, 0x4C, 0x8B, 0xC6, 0x49, 0x8B, 0xD5, 0xFF, 0x50, 0x08
	};
	Code junction(64);
	junction.sub(RSP, 0x28);
	junction.write(ORIGINAL_CODE);
	junction.mov(RCX, RBP); // rbp
	junction.mov(RDX, R15); // packetId
	junction.call(onPacketAfter, RAX);
	junction.add(RSP, 0x28);
	junction.ret();

	McftRenamer renamer;
	junction.patchTo(FNNAME(NetworkHandler$_sortAndPacketizeEvents), 0x720,
		ORIGINAL_CODE, RAX, false);
};
void MinecraftFunctionTable::hookOnPacketSend(void(*callback)(NetworkHandler*, const NetworkIdentifier&, Packet*, unsigned char)) noexcept
{
	static const byte ORIGINAL_CODE[] = {
		0x48, 0x8B, 0x81, 0x48, 0x02, 0x00, 0x00, // mov rax,qword ptr ds:[rcx+248]
		0x48, 0x8B, 0xD9, // mov rbx,rcx
		0x41, 0x0F, 0xB6, 0xE9, // movzx ebp,r9b
		0x49, 0x8B, 0xF8, // mov rdi,r8
		0x4C, 0x8B, 0xF2, // mov r14,rdx
	};
	Code junction(64);
	junction.write(ORIGINAL_CODE + 7, 13);
	junction.sub(RSP, 0x28);
	junction.call(callback, RAX);
	junction.add(RSP, 0x28);
	junction.mov(RAX, QwordPtr, RBX, 0x248);
	junction.mov(R8, RDI);
	junction.ret();

	McftRenamer renamer;
	junction.patchTo(FNNAME(NetworkHandler$send), 0x1A,
		ORIGINAL_CODE, RAX, false);
};
void MinecraftFunctionTable::hookOnPacketSendInternal(NetworkHandler::Connection* (*callback)(NetworkHandler*, const NetworkIdentifier&, Packet*, String*)) noexcept
{
	/*
	mov r14,r9
	mov rdi,r8
	mov rbp,rdx
	mov rsi,rcx
	call <bedrock_server.private: class NetworkHandler::Connection * __ptr64 __cdecl NetworkHandler::_getConnectionFromId(class NetworkIdentifier const & __ptr64)const __ptr64>
	*/
	static const byte ORIGINAL_CODE[] = {
		0x4D, 0x8B, 0xF1, 0x49, 0x8B, 0xF8, 0x48, 0x8B, 0xEA, 0x48, 0x8B, 0xF1, 0xE8, 0xAB, 0xE3, 0xFF, 0xFF
	};
	Code junction(64);
	junction.mov(R14, R9);
	junction.mov(RDI, R8);
	junction.sub(RBP, RDX);
	junction.mov(RSI, RCX);
	junction.add(RSP, 0x28);
	junction.call(callback, RAX);
	junction.add(RSP, 0x28);
	junction.ret();

	McftRenamer renamer;
	// junction.patchTo(FNNAME(NetworkHandler$_sendInternal), 0x14,
	//	ORIGINAL_CODE, RAX, false, { {10, 14} });
};
void MinecraftFunctionTable::hookOnConnectionClosedAfter(void(*onclose)(const NetworkIdentifier&)) noexcept
{
	/*
	pop rbp
	pop rbx
	ret 
	int3 
	int3 
	int3 
	int3 
	int3 
	int3 
	int3 
	int3 
	int3 
	int3 
	*/
	static const byte ORIGINAL_CODE[] = {
		0x5D, 0x5B, 0xC3, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
	};
	Code junction(64);
	junction.mov(RCX, RBP); // networkIdentifier
	junction.pop(RBP);
	junction.pop(RBX);
	junction.jump(onclose, RAX);

	McftRenamer renamer;
	junction.patchTo(FNNAME(NetworkHandler$onConnectionClosed), 0xE3,
		ORIGINAL_CODE, RAX, true);
}
void MinecraftFunctionTable::hookOnRuntimeError(void(*callback)(EXCEPTION_POINTERS* ptr)) noexcept
{
	static const byte ORIGINAL_CODE[] = {
		0x41, 0xFF, 0xD2,	// call r10
		0x48, 0x8B, 0x8B, 0x20, 0x01, 0x00, 0x00, // mov rcx,qword ptr[rbx + 120h]
		0x45, 0x33, 0xC0,	// xor r8d,r8d
	};

	{
		void* target = (byte*)google_breakpad$ExceptionHandler$HandleException;
		Unprotector unpro(target, 12);
		CodeWriter code(target, 12);
		code.jump(callback, RAX);
	}

	static void(* s_callback)(EXCEPTION_POINTERS * ptr) = callback;

	void (*onInvalidParameter)() = []() {
		EXCEPTION_RECORD exception_record = {};
		CONTEXT exception_context = {};
		EXCEPTION_POINTERS exception_ptrs = { &exception_record, &exception_context };
		::RtlCaptureContext(&exception_context);
		exception_record.ExceptionCode = STATUS_INVALID_PARAMETER;

		// We store pointers to the the expression and function strings,
		// and the line as exception parameters to make them easy to
		// access by the developer on the far side.
		exception_record.NumberParameters = 3;
		exception_record.ExceptionInformation[0] = 0;
		exception_record.ExceptionInformation[1] = 0;
		exception_record.ExceptionInformation[2] = 0;
		s_callback(&exception_ptrs);
	};
	{
		void* target = google_breakpad$ExceptionHandler$HandleInvalidParameter;
		Unprotector unpro(target, 12);
		CodeWriter code(target, 12);
		code.jump(onInvalidParameter, RAX);
	}
};
void MinecraftFunctionTable::hookOnCommand(intptr_t(*callback)(MinecraftCommands* commands, MCRESULT* res, SharedPtr<CommandContext>* ctx, bool)) noexcept
{
	static const byte ORIGINAL_CODE[] = {
		0x4C, 0x89, 0x45, 0xB0, // mov qword ptr ss:[rbp-50],r8
		0x49, 0x8B, 0x00, // mov rax,qword ptr ds:[r8]
		0x48, 0x8B, 0x48, 0x20, // mov rcx,qword ptr ds:[rax+20]
		0x48, 0x8B, 0x01, // mov rax,qword ptr ds:[rcx]
	};

	Code junction(96);
	junction.mov(RCX, RSP);
	junction.sub(RSP, 0x28);
	junction.call(callback, RAX);
	junction.add(RSP, 0x28);
	junction.test(RAX, RAX);
	junction.jz(13);
	junction.pop(RCX);
	junction.jump64((byte*)MinecraftCommands$executeCommand + 0x73b, RAX);
	junction.mov(QwordPtr, RBP, -0x50, RSI);
	junction.mov(RAX, QwordPtr, RSI);
	junction.mov(RCX, QwordPtr, RAX, 0x20);
	junction.mov(RAX, QwordPtr, RCX);
	junction.ret();

	McftRenamer renamer;
	junction.patchTo(
		FNNAME(MinecraftCommands$executeCommand), 0x40,
		ORIGINAL_CODE, RAX, false);
};
void MinecraftFunctionTable::hookOnLog(void(*callback)(int color, const char* log, size_t size)) noexcept
{
	static const byte ORIGINAL_CODE[] = {
		0xB9, 0xF5, 0xFF, 0xFF, 0xFF,			//	| mov ecx,FFFFFFF5                                                    |
		0xFF, 0x15, 0x33, 0x1B, 0xE4, 0x00,		//	| call qword ptr ds:[<&GetStdHandle>]                                 |
		0x83, 0xFF, 0x01,						//	| cmp edi,1                                                           |
		0x75, 0x05,								//	| jne bedrock_server.7FF786A2273F                                     |
		0x8D, 0x55, 0x08, 						//	| lea edx,qword ptr ss:[rbp+8]                                        |
		0xEB, 0x19,								//	| jmp bedrock_server.7FF786A22758                                     |
		0x83, 0xFF, 0x02, 						//	| cmp edi,2                                                           |
		0x75, 0x05,								//	| jne bedrock_server.7FF786A22749                                     |
		0x8D, 0x57, 0x0D, 						//	| lea edx,qword ptr ds:[rdi+D]                                        |
		0xEB, 0x0F,								//	| jmp bedrock_server.7FF786A22758                                     |
		0xBA, 0x0E, 0x00, 0x00, 0x00,			//	| mov edx,E                                                           |
		0x83, 0xFF, 0x04,						//	| cmp edi,4                                                           |
		0x74, 0x05,								//	| je bedrock_server.7FF786A22758                                      |
		0xBA, 0x0C, 0x00, 0x00, 0x00,			//	| mov edx,C                                                           | C:'\f'
		0x48, 0x8B, 0xC8, 						//	| mov rcx,rax                                                         |
		0xFF, 0x15, 0x0F, 0x1B, 0xE4, 0x00,		//	| call qword ptr ds:[<&SetConsoleTextAttribute>]                      |
		0x48, 0x8D, 0x54, 0x24, 0x50,			//	| lea rdx,qword ptr ss:[rsp+50]                                       | [rsp+50]:"LdrpInitializeProcess"
		0x48, 0x8D, 0x0D, 0xC7, 0xF6, 0xEF, 0x00, // | lea rcx,qword ptr ds:[7FF787921E34]                                 | 00007FF787921E34:"%s"
		0xE8, 0x3E, 0x86, 0xFC, 0xFF,			//	| call <bedrock_server.printf>                                        |
		0x48, 0x8D, 0x4C, 0x24, 0x50,			//	| lea rcx,qword ptr ss:[rsp+50]                                       | [rsp+50]:"LdrpInitializeProcess"
		0xFF, 0x15, 0x83, 0x1A, 0xE4, 0x00,		//	| call qword ptr ds:[<&OutputDebugStringA>]                           |
	};

	Code junction(64);
	junction.lea(RDX, RSP, 0x58);
	junction.sub(RSP, 0x28);
	junction.mov(RCX, RDI);
	junction.mov(R8, RAX);
	junction.call(callback, RAX);
	junction.add(RSP, 0x28);
	junction.ret();

	McftRenamer renamer;
	junction.patchTo(
		FNNAME(BedrockLogOut), 0x8A,
		ORIGINAL_CODE, RDX, false, { {7, 11},  {51, 55},  {63, 67},  {68, 72},  {79, 83}, });
}
void MinecraftFunctionTable::hookOnCommandPrint(void(*callback)(const char* log, size_t size)) noexcept
{
	/*
	call <bedrock_server.class std::basic_ostream<char,struct std::char_traits<char> > & __ptr64 __cdecl std::_Insert_string<char,struct std::char_traits<char>,unsigned __int64>(class std::basic_ostream<char,struct std::char_traits<char> > & __ptr64,char const * __ptr64 const,uns>
	lea rdx,qword ptr ds:[<class std::basic_ostream<char,struct std::char_traits<char> > & __ptr64 __cdecl std::flush<char,struct std::char_traits<char> >(class std::basic_ostream<char,struct std::char_traits<char> > & __ptr64)>]
	mov rcx,rax
	call qword ptr ds:[<&??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z>]
	*/
	static const byte ORIGINAL_CODE[] = {
		0xE8, 0xFF, 0xFF, 0xFF, 0xFF,
		0x48, 0x8D, 0x15, 0xFF, 0xFF, 0xFF, 0xFF, 
		0x48, 0x8B, 0xC8, 
		0xFF, 0x15, 0xFF, 0xFF, 0xFF, 0xFF,
	};

	Code junction(64);
	junction.sub(RSP, 0x28);
	junction.mov(RCX, RDX);
	junction.mov(RDX, R8);
	junction.call(callback, RAX);
	junction.add(RSP, 0x28);
	junction.ret();

	McftRenamer renamer;
	junction.patchTo(FNNAME(CommandOutputSender$send), 0x1b3, ORIGINAL_CODE, RAX, false, { {1, 5},  {8, 12},  {17, 21}, });
}
void MinecraftFunctionTable::hookOnCommandIn(void(*callback)(String* dest)) noexcept
{
	static const byte ORIGINAL_CODE[] = {
		0x48, 0x8B, 0x1D, 0x00, 0x00, 0x00, 0x00,	// mov rbx,qword ptr ds:[<&?cin@std@@3V?$basic_istream@DU?$char_traits@D@std@@@1@A>]  
		0x48, 0x8B, 0x03,							// mov rax,qword ptr ds:[rbx]                                                         
		0x48, 0x63, 0x48, 0x04,						// movsxd rcx,dword ptr ds:[rax+4]                                                    
		0x48, 0x03, 0xCB,							// add rcx,rbx                                                                        
		0xB2, 0x0A,									// mov dl,A                                                                           
		0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,			// call qword ptr ds:[<&?widen@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBADD@Z>]    
		0x44, 0x0F, 0xB6, 0xC0,						// movzx r8d,al                                                                       
		0x48, 0x8D, 0x54, 0x24, 0x28,				// lea rdx,qword ptr ss:[rsp+28]                                                      
		0x48, 0x8B, 0xCB,							// mov rcx,rbx                                                                        
		0xE8, 0xFF, 0xFF, 0xFF, 0xFF,				// call <bedrock_server.class std::basic_istream<char,struct std::char_traits<char> > 
		0x48, 0x8B, 0x08,							// mov rcx,qword ptr ds:[rax]
		0x48, 0x63, 0x51, 0x04,						// movsxd rdx,dword ptr ds:[rcx+4]
		0xF6, 0x44, 0x02, 0x10, 0x06,				// test byte ptr ds:[rdx+rax+10],6
		0x0F, 0x85, 0xB1, 0x00, 0x00, 0x00,			// jne bedrock_server.7FF6C7A743AC
	};
	Code junction(64);
	junction.lea(RCX, RSP, 0x30);
	junction.sub(RSP, 0x28);
	junction.call(callback, RAX);
	junction.add(RSP, 0x28);
	junction.ret();

	McftRenamer renamer;
	junction.patchTo(FNNAME(std$_LaunchPad$_stdin_t_$_Execute$_0_), 0x5f, ORIGINAL_CODE, RAX, false,
		{ {3, 7}, {21, 25}, {38, 42} });
}
void MinecraftFunctionTable::skipPacketViolationWhen7f() noexcept
{
	/*
	mov qword ptr ss:[rsp+10],rbx
	push rbp
	push rsi
	push rdi
	push r12
	push r13
	push r14
	*/
	static const byte ORIGINAL_CODE[] = {
		0x48, 0x89, 0x5C, 0x24, 0x10, 0x55, 0x56, 0x57, 
		0x41, 0x54, 0x41, 0x55, 0x41, 0x56
	};

	Code junction(64);
	junction.cmp(R8, 0x7f);
	junction.jnz(9);
	junction.mov(RAX, QwordPtr, RSP, 0x28);
	junction.mov(BytePtr, RAX, 0, 0);
	junction.ret();
	junction.write(ORIGINAL_CODE);
	junction.jump((byte*)PacketViolationHandler$_handleViolation + sizeof(ORIGINAL_CODE), RAX);

	McftRenamer renamer;
	junction.patchTo(FNNAME(PacketViolationHandler$_handleViolation), 0, ORIGINAL_CODE, RAX, true,
		{ {3, 7}, {21, 24} });
}

void MinecraftFunctionTable::forceEnableScript() noexcept
{
	size_t targetSize = 16;
	Unprotector unpro((byte*)MinecraftServerScriptEngine$onServerThreadStarted, targetSize);
	CodeWriter writer((void*)unpro, targetSize);
	writer.mov(RAX, (dword)1);
	writer.ret();
}
