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

#include "kyra/engine/util.h"
#include "kyra/resource/resource.h"
#include "kyra/resource/resource_intern.h"

#include "common/config-manager.h"
#include "common/fs.h"
#include "common/substream.h"

namespace Kyra {

Resource::Resource(KyraEngine_v1 *vm) : _archiveCache(), _files(), _archiveFiles(), _protectedFiles(), _loaders(), _vm(vm) {
	initializeLoaders();

	// Initialize directories for playing with the original directory structure
	SearchMan.addSubDirectoryMatching(Common::FSNode(ConfMan.getPath("path")), "data", 0, 2);

	_files.add("global_search", &Common::SearchManager::instance(), 3, false);
	// compressed installer archives are added at level '2',
	// but that's done in Resource::reset not here
	_files.add("protected", &_protectedFiles, 1, false);
	_files.add("archives", &_archiveFiles, 0, false);
}

Resource::~Resource() {
	_loaders.clear();

	for (ArchiveMap::iterator i = _archiveCache.begin(); i != _archiveCache.end(); ++i)
		delete i->_value;
	_archiveCache.clear();
}

bool Resource::reset() {
	unloadAllPakFiles();

	Common::FSNode dir(ConfMan.getPath("path"));

	if (!dir.exists() || !dir.isDirectory())
		error("invalid game path '%s'", dir.getPath().toString(Common::Path::kNativeSeparator).c_str());

	if (_vm->gameFlags().useInstallerPackage)
		_files.add("installer", loadInstallerArchive("WESTWOOD", "%d", 0), 2, false);

	if (!_vm->gameFlags().isTalkie) {
		static const char *const list[] = {
			"GENERAL.PAK", nullptr
		};

		loadProtectedFiles(list);
	}

	return true;
}

bool Resource::loadPakFile(const Common::Path &filename) {
	Common::Path filenameFixed(filename);
	filenameFixed.toUppercase();

	Common::ArchiveMemberPtr file = _files.getMember(filenameFixed);
	if (!file)
		return false;

	return loadPakFile(filenameFixed.toString('/'), file);
}

bool Resource::loadPakFile(const Common::String &name, Common::ArchiveMemberPtr file) {
	Common::String nameFixed(name);
	nameFixed.toUppercase();

	if (_archiveFiles.hasArchive(nameFixed) || _protectedFiles.hasArchive(nameFixed))
		return true;

	Common::Archive *archive = loadArchive(nameFixed, file);
	if (!archive)
		return false;

	_archiveFiles.add(nameFixed, archive, 0, false);

	return true;
}

bool Resource::loadFileList(const Common::Path &filedata) {
	Common::SeekableReadStream *f = createReadStream(filedata);

	if (!f)
		return false;

	uint32 filenameOffset = 0;
	while ((filenameOffset = f->readUint32LE()) != 0) {
		uint32 offset = f->pos();
		f->seek(filenameOffset, SEEK_SET);

		uint8 buffer[13];
		f->read(buffer, sizeof(buffer) - 1);
		buffer[12] = 0;
		f->seek(offset + 16, SEEK_SET);

		Common::String filename = Common::String((char *)buffer);
		filename.toUppercase();

		if (filename.hasSuffix(".PAK")) {
			Common::Path path(filename);
			if (!loadPakFile(path)) {
				delete f;
				error("couldn't load file '%s'", filename.c_str());
				return false;   // for compilers that don't support NORETURN
			}
		}
	}

	delete f;
	return true;
}

bool Resource::loadFileList(const char *const *filelist, uint32 numFiles) {
	if (!filelist)
		return false;

	while (numFiles--) {
		if (!loadPakFile(filelist[numFiles])) {
			error("couldn't load file '%s'", filelist[numFiles]);
			return false;   // for compilers that don't support NORETURN
		}
	}

	return true;
}

bool Resource::loadProtectedFiles(const char *const *list) {
	for (uint i = 0; list[i]; ++i) {
		Common::ArchiveMemberPtr file = _files.getMember(list[i]);
		if (!file)
			error("Couldn't find PAK file '%s'", list[i]);

		Common::Archive *archive = loadArchive(list[i], file);
		if (archive)
			_protectedFiles.add(list[i], archive, 0, false);
		else
			error("Couldn't load PAK file '%s'", list[i]);
	}

	return true;
}

void Resource::unloadPakFile(const Common::String &name, bool remFromCache) {
	Common::String nameFixed(name);
	nameFixed.toUppercase();

	// We do not remove files from '_protectedFiles' here, since
	// those are protected against unloading.
	if (_archiveFiles.hasArchive(nameFixed)) {
		_archiveFiles.remove(nameFixed);
		if (remFromCache) {
			ArchiveMap::iterator iter = _archiveCache.find(nameFixed);
			if (iter != _archiveCache.end()) {
				delete iter->_value;
				_archiveCache.erase(nameFixed);
			}
		}
	}
}

bool Resource::isInCacheList(const Common::String &name) {
	Common::String nameFixed(name);
	nameFixed.toUppercase();
	return (_archiveCache.find(nameFixed) != _archiveCache.end());
}

void Resource::unloadAllPakFiles() {
	_archiveFiles.clear();
	_protectedFiles.clear();
}

void Resource::listFiles(const Common::Path &pattern, Common::ArchiveMemberList &list) {
	_files.listMatchingMembers(list, pattern);
}

uint8 *Resource::fileData(const Common::Path &file, uint32 *size) {
	Common::SeekableReadStream *stream = createReadStream(file);
	if (!stream)
		return nullptr;

	uint32 bufferSize = stream->size();
	uint8 *buffer = new uint8[bufferSize];
	assert(buffer);
	if (size)
		*size = bufferSize;
	stream->read(buffer, bufferSize);
	delete stream;
	return buffer;
}

bool Resource::exists(const Common::Path &file, bool errorOutOnFail) {
	if (_files.hasFile(file))
		return true;
	else if (errorOutOnFail)
		error("File '%s' can't be found", file.toString().c_str());
	return false;
}

uint32 Resource::getFileSize(const Common::Path &file) {
	Common::SeekableReadStream *stream = createReadStream(file);
	if (!stream)
		return 0;

	uint32 size = stream->size();
	delete stream;
	return size;
}

bool Resource::loadFileToBuf(const Common::Path &file, void *buf, uint32 maxSize) {
	Common::SeekableReadStream *stream = createReadStream(file);
	if (!stream)
		return false;

	memset(buf, 0, maxSize);
	stream->read(buf, ((int32)maxSize <= stream->size()) ? maxSize : stream->size());
	delete stream;
	return true;
}

Common::SeekableReadStream *Resource::createReadStream(const Common::Path &file) {
	return _files.createReadStreamForMember(file);
}

Common::Archive *Resource::loadArchive(const Common::String &name, Common::ArchiveMemberPtr member) {
	ArchiveMap::iterator cachedArchive = _archiveCache.find(name);
	if (cachedArchive != _archiveCache.end())
		return cachedArchive->_value;

	Common::SeekableReadStream *stream = member->createReadStream();

	if (!stream)
		return nullptr;

	Common::Archive *archive = nullptr;
	for (LoaderList::const_iterator i = _loaders.begin(); i != _loaders.end(); ++i) {
		if ((*i)->checkFilename(name)) {
			if ((*i)->isLoadable(name, *stream)) {
				stream->seek(0, SEEK_SET);
				archive = (*i)->load(member, *stream);
				break;
			} else {
				stream->seek(0, SEEK_SET);
			}
		}
	}

	delete stream;

	if (!archive)
		return nullptr;

	_archiveCache[name] = archive;
	return archive;
}

Common::Archive *Resource::loadInstallerArchive(const Common::Path &file, const Common::String &ext, const uint8 offset) {
	Common::String name(file.toString('/'));
	ArchiveMap::iterator cachedArchive = _archiveCache.find(name);
	if (cachedArchive != _archiveCache.end())
		return cachedArchive->_value;

	Common::Archive *archive = InstallerLoader::load(this, file, ext, offset);
	if (!archive)
		return nullptr;

	_archiveCache[name] = archive;
	return archive;
}

#pragma mark -

void Resource::initializeLoaders() {
	_loaders.push_back(LoaderList::value_type(new ResLoaderPak()));
	_loaders.push_back(LoaderList::value_type(new ResLoaderTlk()));
}

} // End of namespace Kyra
