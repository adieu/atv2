#include "FileUtils.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogKeyboard.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "JobManager.h"
#include "FileOperationJob.h"
#include "Util.h"
#include "FileSystem/MultiPathDirectory.h"
#include <vector>
#include <openssl/md5.h>

using namespace XFILE;
using namespace std;

bool CFileUtils::DeleteItem(const CFileItemPtr &item)
{
  if (!item)
    return false;

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (pDialog)
  {
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 125);
    pDialog->SetLine(1, CUtil::GetFileName(item->m_strPath));
    pDialog->SetLine(2, "");
    pDialog->DoModal();
    if (!pDialog->IsConfirmed()) return false;
  }

  // Create a temporary item list containing the file/folder for deletion
  CFileItemPtr pItemTemp(new CFileItem(*item));
  pItemTemp->Select(true);
  CFileItemList items;
  items.Add(pItemTemp);

  // grab the real filemanager window, set up the progress bar,
  // and process the delete action
  CFileOperationJob op(CFileOperationJob::ActionDelete, items, "");

  return op.DoWork();
}

bool CFileUtils::RenameFile(const CStdString &strFile)
{
  CStdString strFileAndPath(strFile);
  CUtil::RemoveSlashAtEnd(strFileAndPath);
  CStdString strFileName = CUtil::GetFileName(strFileAndPath);
  CStdString strPath = strFile.Left(strFileAndPath.size() - strFileName.size());
  if (CGUIDialogKeyboard::ShowAndGetInput(strFileName, g_localizeStrings.Get(16013), false))
  {
    strPath += strFileName;
    CLog::Log(LOGINFO,"FileUtils: rename %s->%s\n", strFileAndPath.c_str(), strPath.c_str());
    if (CUtil::IsMultiPath(strFileAndPath))
    { // special case for multipath renames - rename all the paths.
      vector<CStdString> paths;
      CMultiPathDirectory::GetPaths(strFileAndPath, paths);
      bool success = false;
      for (unsigned int i = 0; i < paths.size(); ++i)
      {
        CStdString filePath(paths[i]);
        CUtil::RemoveSlashAtEnd(filePath);
        CUtil::GetDirectory(filePath, filePath);
        CUtil::AddFileToFolder(filePath, strFileName, filePath);
        if (CFile::Rename(paths[i], filePath))
          success = true;
      }
      return success;
    }
    return CFile::Rename(strFileAndPath, strPath);
  }
  return false;
}


bool CFileUtils::SubtitleFileSizeAndHash(const CStdString &path, CStdString &strSize, CStdString &strHash)
{
  const size_t chksum_block_size = 8192;
  
  CFile file;
  size_t i;
  uint64_t hash = 0;
  uint64_t buffer1[chksum_block_size*2];
  uint64_t fileSize ;
  // In natural language it calculates: size + 64k chksum of the first and last 64k
  // (even if they overlap because the file is smaller than 128k).
  file.Open(path, READ_NO_CACHE); //open file
  file.Read(buffer1, chksum_block_size*sizeof(uint64_t)); //read first 64k
  file.Seek(-(int64_t)chksum_block_size*sizeof(uint64_t), SEEK_END); //seek to the end of the file
  file.Read(&buffer1[chksum_block_size], chksum_block_size*sizeof(uint64_t)); //read last 64k

  for (i=0;i<chksum_block_size*2;i++)
    hash += buffer1[i];
  
  fileSize = file.GetLength();
  
  hash += fileSize; //add size

  file.Close(); //close file
  strHash.Format("%"PRIx64"", hash);
  strSize.Format("%d", fileSize);
  return true;
}

bool CFileUtils::SubtitleFileSizeAndHash2(const CStdString &path, CStdString &digest1, CStdString &digest2, CStdString &digest3, CStdString &digest4)
{
  const size_t chksum_block_size = 4096;

  CFile file;
  size_t i;
  uint8_t buffer1[chksum_block_size*4];
  uint64_t fileSize ;
  unsigned char * tmp_hash;
  char digest[MD5_DIGEST_LENGTH*2];
  // In natural language it calculates: size + 64k chksum of the first and last 64k
  // (even if they overlap because the file is smaller than 128k).

  file.Open(path, READ_NO_CACHE); //open file
  fileSize = file.GetLength();
  file.Seek((int64_t)chksum_block_size*sizeof(uint8_t)); //seek to the end of the file
  file.Read(buffer1, chksum_block_size*sizeof(uint8_t)); //read first 64k
  file.Seek((int64_t)fileSize/3*2); //seek to the end of the file
  file.Read(&buffer1[chksum_block_size], chksum_block_size*sizeof(uint8_t)); //read last 64k
  file.Seek( (int64_t)fileSize/3); //seek to the end of the file
  file.Read(&buffer1[chksum_block_size*2], chksum_block_size*sizeof(uint8_t)); //read last 64k
  file.Seek(-(int64_t)chksum_block_size*sizeof(uint8_t)*2, SEEK_END); //seek to the end of the file
  file.Read(&buffer1[chksum_block_size*3], chksum_block_size*sizeof(uint8_t)); //read last 64k

  file.Close(); //close file

  tmp_hash = MD5((const unsigned char*)&buffer1[0], chksum_block_size, NULL);
  for (i=0;i<MD5_DIGEST_LENGTH;i++) {
    sprintf(&digest[i*2], "%02x", tmp_hash[i]);
  }
  digest1.Format("%s", digest);

  tmp_hash = MD5((const unsigned char*)&buffer1[chksum_block_size], chksum_block_size, NULL);
  for (i=0;i<MD5_DIGEST_LENGTH;i++) {
    sprintf(&digest[i*2], "%02x", tmp_hash[i]);
  }
  digest2.Format("%s", digest);

  tmp_hash = MD5((const unsigned char*)&buffer1[chksum_block_size*2], chksum_block_size, NULL);
  for (i=0;i<MD5_DIGEST_LENGTH;i++) {
    sprintf(&digest[i*2], "%02x", tmp_hash[i]);
  }
  digest3.Format("%s", digest);

  tmp_hash = MD5((const unsigned char*)&buffer1[chksum_block_size*3], chksum_block_size, NULL);
  for (i=0;i<MD5_DIGEST_LENGTH;i++) {
    sprintf(&digest[i*2], "%02x", tmp_hash[i]);
  }
  digest4.Format("%s", digest);

  return true;
}

