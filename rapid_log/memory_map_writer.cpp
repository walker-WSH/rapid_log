#include "memory_map_writer.h"
#include <windows.h>
#include <assert.h>

#define memfile_block_size (64 * 1024)

//------------------------------------------------------------------------------------
class CMemoryMapWriter::impl {
	HANDLE handle_file_; //	�򿪾��
	HANDLE handle_map_;  //	ӳ����

	unsigned int original_size_;    //  ԭʼ��С
	unsigned int file_size_;        //	�ļ���С
	unsigned int cur_block_pos_;    //	��ǰӳ���ĵ�һ���ֽ����ļ��е�ƫ��
	char *cur_block_ptr_;           //	��ǰӳ���ָ��
	unsigned int cur_block_size_;   //	��ǰӳ���Ĵ�С
	unsigned int cur_block_offset_; //	��ǰӳ����дλ��

public:
	impl();
	~impl();

public:
	bool Open(const TCHAR *path, bool cover_write);
	void Close();
	unsigned int WriteData(const void *ptr, const unsigned int &len);

private:
	bool i_OpenFile(const TCHAR *path, bool cover_write);
	bool i_CreateFileMap();
	void i_MapNextBlock();
	void i_ExpandFileSize();
	void i_CloseFileMap();
	void i_CloseFile();
};

//------------------------------------------------------------------------
CMemoryMapWriter::impl::impl()
	: handle_file_(), handle_map_(), original_size_(), file_size_(), cur_block_pos_(), cur_block_ptr_(), cur_block_size_(), cur_block_offset_()
{
}

CMemoryMapWriter::impl::~impl()
{
	Close();
}

bool CMemoryMapWriter::impl::Open(const TCHAR *path, bool cover_write)
{
	return i_OpenFile(path, cover_write);
}

void CMemoryMapWriter::impl::Close()
{
	i_CloseFileMap();
	i_CloseFile();
}

void CMemoryMapWriter::impl::i_CloseFileMap()
{
	if (cur_block_ptr_)
		::UnmapViewOfFile(cur_block_ptr_);
	if (handle_map_)
		::CloseHandle(handle_map_);
	handle_map_ = 0;
	cur_block_ptr_ = 0;
}

void CMemoryMapWriter::impl::i_CloseFile()
{
	if (handle_file_ && (handle_file_ != INVALID_HANDLE_VALUE)) {
		::SetFilePointer(handle_file_, original_size_ + cur_block_pos_ + cur_block_offset_, 0, FILE_BEGIN);
		::SetEndOfFile(handle_file_);
		::CloseHandle(handle_file_);
	}
	original_size_ = 0;
	handle_file_ = 0;
	file_size_ = 0;
	cur_block_ptr_ = 0;
	cur_block_pos_ = 0;
	cur_block_size_ = 0;
	cur_block_offset_ = 0;
}

bool CMemoryMapWriter::impl::i_OpenFile(const TCHAR *path, bool cover_write)
{
	handle_file_ = ::CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (!handle_file_ || handle_file_ == INVALID_HANDLE_VALUE) {
		handle_file_ = ::CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
	}

	if (!handle_file_ || handle_file_ == INVALID_HANDLE_VALUE) {
		handle_file_ = 0;
		return false;
	}

	if (cover_write) {
		::SetFilePointer(handle_file_, 0, 0, FILE_BEGIN);
		::SetEndOfFile(handle_file_);
	}

	file_size_ = ::GetFileSize(handle_file_, 0);
	original_size_ = file_size_;
	if (file_size_ == 0) {
		file_size_ = memfile_block_size;
		::SetFilePointer(handle_file_, file_size_, 0, FILE_BEGIN);
		::SetEndOfFile(handle_file_);
	}

	return i_CreateFileMap();
}

bool CMemoryMapWriter::impl::i_CreateFileMap()
{
	file_size_ = ::GetFileSize(handle_file_, 0);
	if (file_size_ == 0xFFFFFFFF) {
		Close(); // ���ֻд4G
		return false;
	}

	handle_map_ = ::CreateFileMapping(handle_file_, 0, PAGE_READWRITE, 0, file_size_, 0);
	if (handle_map_ == INVALID_HANDLE_VALUE) {
		Close();
		return false;
	}

	cur_block_size_ = memfile_block_size;
	cur_block_offset_ = 0;
	cur_block_ptr_ = (char *)::MapViewOfFile(handle_map_, FILE_MAP_READ | FILE_MAP_WRITE, 0, cur_block_pos_, cur_block_size_);
	// ���׷�ӵ��ļ����ݳ��Ȳ�Ϊ�� ��С��64K ��˴��᷵��NULL

	assert(cur_block_ptr_);
	if (!cur_block_ptr_) {
		Close();
		return false;
	}

	return true;
}

unsigned int CMemoryMapWriter::impl::WriteData(const void *ptr, const unsigned int &len)
{
	if (!cur_block_ptr_ || !ptr)
		return 0;

	const char *p = (const char *)ptr;
	unsigned int w = len;
	while (w > 0) {
		unsigned int tow = cur_block_size_ - cur_block_offset_;
		if (tow > w)
			tow = w;
		memcpy(cur_block_ptr_ + cur_block_offset_, p, tow);
		cur_block_offset_ += tow;
		p += tow;
		w -= tow;
		if (cur_block_size_ == cur_block_offset_)
			i_MapNextBlock();
		if (cur_block_ptr_ == 0)
			break;
	}
	return len - w;
}

void CMemoryMapWriter::impl::i_MapNextBlock()
{
	if (cur_block_ptr_)
		::UnmapViewOfFile(cur_block_ptr_);
	cur_block_ptr_ = 0;

	cur_block_pos_ += cur_block_size_;
	cur_block_ptr_ = (char *)::MapViewOfFile(handle_map_, FILE_MAP_READ | FILE_MAP_WRITE, 0, cur_block_pos_, cur_block_size_);
	cur_block_offset_ = 0;

	if (cur_block_ptr_ == 0)
		i_ExpandFileSize();
}

void CMemoryMapWriter::impl::i_ExpandFileSize()
{
	i_CloseFileMap();

	file_size_ = ::GetFileSize(handle_file_, 0);
	if (file_size_ == 0xFFFFFFFF)
		return;

	file_size_ += memfile_block_size;
	::SetFilePointer(handle_file_, file_size_, 0, FILE_BEGIN);
	::SetEndOfFile(handle_file_);

	i_CreateFileMap();
}

//------------------------------------------------------------
CMemoryMapWriter::CMemoryMapWriter()
{
	self = new impl();
}

CMemoryMapWriter::~CMemoryMapWriter()
{
	delete self;
}

bool CMemoryMapWriter::Open(const TCHAR *path, bool cover_write)
{
	return self->Open(path, cover_write);
}

void CMemoryMapWriter::Close()
{
	self->Close();
}

unsigned int CMemoryMapWriter::WriteData(const void *ptr, const unsigned int &len)
{
	return self->WriteData(ptr, len);
}