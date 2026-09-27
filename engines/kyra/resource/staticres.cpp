/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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
 *
 */

#include "kyra/resource/resource.h"
#include "kyra/graphics/screen.h"

#include "common/endian.h"
#include "common/md5.h"

namespace Kyra {

#define RESFILE_VERSION 123

namespace {
bool checkKyraDat(Common::SeekableReadStream *file) {
	if (!file)
		return false;

	uint32 size = file->size() - 16;
	uint8 digest[16];
	file->seek(size, SEEK_SET);
	if (file->read(digest, 16) != 16)
		return false;

	uint8 digestCalc[16];
	file->seek(0, SEEK_SET);
	if (!Common::computeStreamMD5(*file, digestCalc, size))
		return false;

	for (int i = 0; i < 16; ++i)
		if (digest[i] != digestCalc[i])
			return false;
	return true;
}

struct IndexTable {
	int type;
	int value;

	bool operator==(int t) const {
		return (type == t);
	}
};

// The value is the game index used in the KYRA.DAT file format (see devtools/create_kyradat).
const IndexTable iGameTable[] = {
	{ GI_LOL, 5 },
	{ -1, -1 }
};

byte getGameID(const GameFlags &flags) {
	return Common::find(iGameTable, ARRAYEND(iGameTable) - 1, flags.gameID)->value;
}

const IndexTable iLanguageTable[] = {
	{ Common::EN_ANY,  1 },
	{ Common::FR_FRA,  2 },
	{ Common::DE_DEU,  3 },
	{ Common::ES_ESP,  4 },
	{ Common::IT_ITA,  5 },
	{ Common::JA_JPN,  6 },
	{ Common::RU_RUS,  7 },
	{ Common::HE_ISR,  8 },
	{ Common::ZH_CHN,  9 },
	{ Common::ZH_TWN, 10 },
	{ Common::KO_KOR, 11 },
	{ Common::CS_CZE, 12 },
	{ Common::PL_POL, 13 },
	{ -1, -1 }
};

byte getLanguageID(Common::Language lang) {
	return Common::find(iLanguageTable, ARRAYEND(iLanguageTable) - 1, lang)->value;
}

// Platform index of DOS in the KYRA.DAT file format.
byte getPlatformID(const GameFlags &) {
	return 0;
}

// Special index in the KYRA.DAT file format: 1 for the CD version, 0 for the floppy version.
byte getSpecialID(const GameFlags &flags) {
	return flags.isTalkie ? 1 : 0;
}

} // end of anonymous namespace

bool StaticResource::loadStaticResourceFile() {
	Resource *res = _vm->resource();

	if (res->isInCacheList(staticDataFilename()))
		return true;

	Common::ArchiveMemberList kyraDatFiles;
	res->listFiles(Common::Path(staticDataFilename()), kyraDatFiles);

	bool foundWorkingKyraDat = false;
	for (Common::ArchiveMemberList::iterator i = kyraDatFiles.begin(); i != kyraDatFiles.end(); ++i) {
		Common::SeekableReadStream *file = (*i)->createReadStream();
		if (!checkKyraDat(file)) {
			delete file;
			continue;
		}

		delete file; file = nullptr;

		if (!res->loadPakFile(staticDataFilename(), *i))
			continue;

		if ((setLanguage(_vm->gameFlags().lang) && prefetchId(-1))) {
			foundWorkingKyraDat = true;
			break;
		}

		res->unloadPakFile(staticDataFilename(), true);
		unloadId(-1);
	}

	if (!foundWorkingKyraDat) {
		Common::String errorMessage = "You're missing the '" + StaticResource::staticDataFilename() + "' engine data file or it got corrupted.";
		GUIErrorMessage(errorMessage);
		error("%s", errorMessage.c_str());
	}

	return true;
}

Common::SeekableReadStream *StaticResource::loadIdMap(Common::Language lang) {
	Common::SeekableReadStream *index = _vm->resource()->createReadStream("INDEX");
	if (!index)
		return 0;

	const uint32 version = index->readUint32BE();

	if (version != RESFILE_VERSION) {
		delete index;
		return 0;
	}

	const uint32 includedGames = index->readUint32BE();

	if (includedGames * 2 + 8 != (uint32)index->size()) {
		delete index;
		return 0;
	}

	const GameFlags &flags = _vm->gameFlags();
	const byte game = getGameID(flags) & 0xF;
	const byte platform = getPlatformID(flags) & 0xF;
	const byte special = getSpecialID(flags) & 0xF;
	const byte lng = getLanguageID(lang) & 0xF;

	const uint16 gameDef = (game << 12) | (platform << 8) | (special << 4) | (lng << 0);

	bool found = false;
	for (uint32 i = 0; i < includedGames; ++i) {
		if (index->readUint16BE() == gameDef) {
			found = true;
			break;
		}
	}

	delete index;
	index = nullptr;

	if (!found)
		return 0;

	// load the ID map for our game
	const Common::Path filenamePattern(Common::String::format("0%01X%01X%01X000%01X", game, platform, special, lng));
	return _vm->resource()->createReadStream(filenamePattern);
}

bool StaticResource::init() {
#define proc(x) &StaticResource::x
	static const FileType fileTypeTable[] = {
		{ kStringList, proc(loadStringTable), proc(freeStringTable) },
		{ kRawData, proc(loadRawData), proc(freeRawData) },

		{ kLoLCharData, proc(loadCharData), proc(freeCharData) },
		{ kLoLSpellData, proc(loadSpellData), proc(freeSpellData) },
		{ kLoLCompassData, proc(loadCompassData), proc(freeCompassData) },
		{ kLoLFlightShpData, proc(loadFlyingObjectData), proc(freeFlyingObjectData) },
		{ kRawDataBe16, proc(loadRawDataBe16), proc(freeRawDataBe16) },
		{ kRawDataBe32, proc(loadRawDataBe32), proc(freeRawDataBe32) },
		{ kLoLButtonData, proc(loadButtonDefs), proc(freeButtonDefs) },

		{ 0, nullptr, nullptr }
	};
#undef proc
	_fileLoader = fileTypeTable;

	return loadStaticResourceFile();
}

void StaticResource::deinit() {
	unloadId(-1);
}

const char *const *StaticResource::loadStrings(int id, int &strings) {
	return (const char *const *)getData(id, kStringList, strings);
}

const uint8 *StaticResource::loadRawData(int id, int &size) {
	return (const uint8 *)getData(id, kRawData, size);
}

const uint16 *StaticResource::loadRawDataBe16(int id, int &entries) {
	return (const uint16 *)getData(id, kRawDataBe16, entries);
}

bool StaticResource::setLanguage(Common::Language lang, int id) {
	if (lang == Common::UNK_LANG)
		lang = _vm->gameFlags().lang;

	unloadId(id);

	// load the ID map for our game
	Common::SeekableReadStream *idMap = loadIdMap(lang);
	if (!idMap)
		return false;

	int numIDs = idMap->readUint16BE();
	while (numIDs--) {
		uint16 id2 = idMap->readUint16BE();
		uint8 type = idMap->readByte();
		uint32 filename = idMap->readUint32BE();
		if (id == -1 || id == id2) {
			_dataTable[id2] = DataDescriptor(filename, type);
			if (id == id2)
				break;
		}
	}

	const bool fileError = idMap->err();
	delete idMap;
	if (fileError || (id != -1 && numIDs == -1))
		return false;

	return true;
}

bool StaticResource::prefetchId(int id) {
	if (id == -1) {
		for (DataMap::const_iterator i = _dataTable.begin(); i != _dataTable.end(); ++i) {
			if (!prefetchId(i->_key))
				return false;
		}
		return true;
	}

	const void *ptr = nullptr;
	int type = -1, size = -1;

	if (checkResList(id, type, ptr, size))
		return true;

	DataMap::const_iterator dDesc = _dataTable.find(id);
	if (dDesc == _dataTable.end())
		return false;

	const FileType *filetype = getFiletype(dDesc->_value.type);
	if (!filetype)
		return false;

	ResData data;
	data.id = id;
	data.type = dDesc->_value.type;
	Common::SeekableReadStream *fileStream = _vm->resource()->createReadStream(Common::Path(Common::String::format("%08X", dDesc->_value.filename)));
	if (!fileStream)
		return false;

	if (!(this->*(filetype->load))(*fileStream, data.data, data.size)) {
		delete fileStream;
		return false;
	}
	delete fileStream;
	_resList.push_back(data);

	return true;
}

void StaticResource::unloadId(int id) {
	Common::List<ResData>::iterator pos = _resList.begin();
	for (; pos != _resList.end();) {
		if (pos->id == id || id == -1) {
			const FileType *filetype = getFiletype(pos->type);
			(this->*(filetype->free))(pos->data, pos->size);
			pos = _resList.erase(pos);
			if (id != -1)
				break;
		} else {
			++pos;
		}
	}
}

bool StaticResource::checkResList(int id, int &type, const void *&ptr, int &size) {
	Common::List<ResData>::iterator pos = _resList.begin();
	for (; pos != _resList.end(); ++pos) {
		if (pos->id == id) {
			size = pos->size;
			type = pos->type;
			ptr = pos->data;
			return true;
		}
	}
	return false;
}

const StaticResource::FileType *StaticResource::getFiletype(int type) {
	if (!_fileLoader)
		return nullptr;

	for (int i = 0; _fileLoader[i].load; ++i) {
		if (_fileLoader[i].type == type)
			return &_fileLoader[i];
	}

	return nullptr;
}

const void *StaticResource::getData(int id, int requesttype, int &size) {
	const void *ptr = nullptr;
	int type = -1;
	size = 0;

	if (checkResList(id, type, ptr, size)) {
		if (type == requesttype)
			return ptr;
		return nullptr;
	}

	if (!prefetchId(id))
		return nullptr;

	if (checkResList(id, type, ptr, size)) {
		if (type == requesttype)
			return ptr;
	}

	return nullptr;
}

bool StaticResource::loadStringTable(Common::SeekableReadStream &stream, void *&ptr, int &size) {
	uint32 count = stream.readUint32BE();
	size = count;
	char **output = new char *[count];
	assert(output);

	for (uint32 i = 0; i < count; ++i) {
		Common::String string;
		char c = 0;
		while ((c = (char)stream.readByte()) != 0)
			string += c;

		output[i] = new char[string.size() + 1];
		Common::strlcpy(output[i], string.c_str(), string.size() + 1);
	}

	ptr = output;
	return true;
}

bool StaticResource::loadRawData(Common::SeekableReadStream &stream, void *&ptr, int &size) {
	ptr = new uint8[stream.size()];
	stream.read(ptr, stream.size());
	size = stream.size();
	return true;
}

bool StaticResource::loadRawDataBe16(Common::SeekableReadStream &stream, void *&ptr, int &size) {
	size = stream.size() >> 1;

	uint16 *r = new uint16[size];

	for (int i = 0; i < size; i++)
		r[i] = stream.readUint16BE();

	ptr = r;
	return true;
}

void StaticResource::freeRawData(void *&ptr, int &size) {
	uint8 *data = (uint8 *)ptr;
	delete[] data;
	ptr = nullptr;
	size = 0;
}

void StaticResource::freeStringTable(void *&ptr, int &size) {
	char **data = (char **)ptr;
	while (size--)
		delete[] data[size];
	delete[] data;
	ptr = nullptr;
	size = 0;
}

void StaticResource::freeRawDataBe16(void *&ptr, int &size) {
	uint16 *data = (uint16 *)ptr;
	delete[] data;
	ptr = 0;
	size = 0;
}

} // End of namespace Kyra
