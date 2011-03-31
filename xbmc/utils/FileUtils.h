#pragma once
#include "FileItem.h"

class CFileUtils
{
public:
  static bool DeleteItem(const CFileItemPtr &item);
  static bool RenameFile(const CStdString &strFile);
  static bool SubtitleFileSizeAndHash(const CStdString &path, CStdString &size, CStdString &hash);
  static bool SubtitleFileSizeAndHash2(const CStdString &path, CStdString &digest1, CStdString &digest2, CStdString &digest3, CStdString &digest4);
};
