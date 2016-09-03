/******************************************************************************/
/*
  Author  - Ming-Lun "Allen" Chou
  Web     - http://AllenChou.net
  Twitter - @TheAllenChou
*/
/******************************************************************************/

/*
  This is a simple implementation of SID database, where data is stored linearly and unsorted.
  Ideally, you'll want to use a search structure like a binary search tree.
  Database file format: Number of data entries (U64) followed by a list of SID-string pairs.
*/

#include "siddb/siddb.h"

#include <unordered_map>

namespace siddb
{

  static std::unordered_map<StringId::Storage, std::string> s_table;
  static std::ostream *s_pMsgOutput;
  static std::string s_logFilePath = "siddb.log";

  void Config(std::ostream &msgOutput, const char *logFilePath)
  {
    s_pMsgOutput = &msgOutput;
    s_logFilePath = logFilePath;
  }

  static void Log(const char *msg)
  {
    *s_pMsgOutput << msg;

    FILE *pFile = std::fopen(s_logFilePath.c_str(), "a+");
    if (!pFile)
    {
      *s_pMsgOutput << "Can't write log to file: " << s_logFilePath;
      return;
    }

    std::fwrite(s_logFilePath.c_str(), 1, s_logFilePath.size(), pFile);
    std::fwrite("\n", 1, 1, pFile);
    std::fclose(pFile);
  }

  void Add(const char *str)
  {
    const StringId::Storage sidVal = SID_VAL(str);
    const auto iter = s_table.find(sidVal);
    if (iter == s_table.end())
    {
      s_table.insert(std::make_pair(sidVal, str));
    }
    else
    {
      char msg[kMaxStrLen * 3];
      std::sprintf(msg, "Hash collision (%016llx):\n%s\n%s\n\n");
      Log(msg);
    }
  }

  void Clean()
  {
    s_table.clear();
  }

  void ForEach(ForEachCallback callback)
  {
    for (const auto &pair : s_table)
    {
      StringId sid(pair.first);
      if (!callback(sid, pair.second.c_str()))
          return;
    }
  }

  size_t GetSize()
  {
    return s_table.size();
  }

  bool Load(const char *filePath)
  {
    FILE *pFile = std::fopen(filePath, "r");
    if (!pFile)
      return false;

    Clean();

    size_t size = 0;
    std::fread(&size, sizeof(size_t), 1, pFile);
    for (size_t i = 0; i < size; ++i)
    {
      StringId::Storage sidVal;
      std::fread(&sidVal, sizeof(StringId::Storage), 1, pFile);
      size_t strLen;
      std::fread(&strLen, sizeof(size_t), 1, pFile);
      char str[kMaxStrLen + 1];
      std::fread(str, 1, strLen, pFile);
      str[strLen] = '\0';

      Add(str);
    }

    std::fclose(pFile);

    return true;
  }

  bool Save(const char *filePath)
  {
    FILE *pFile = std::fopen(filePath, "w+");
    if (!pFile)
      return false;

    const size_t size = s_table.size();
    std::fwrite(&size, sizeof(size_t), 1, pFile);
    for (const auto &pair : s_table)
    {
      std::fwrite(&pair.first, sizeof(StringId::Storage), 1, pFile);
      const size_t strLen = pair.second.size();
      std::fwrite(&strLen, sizeof(size_t), 1, pFile);
      std::fwrite(pair.second.c_str(), 1, strLen, pFile);
    }

    std::fclose(pFile);

    return true;
  }

  bool Find(StringId sid, char *pOut)
  {
    const auto iter = s_table.find(sid.GetValue());
    if (iter != s_table.end())
    {
      std::strcpy(pOut, iter->second.c_str());

      return true;
    }

    return false;
  }

  bool Find(const char *str, StringId *pOut)
  {
    const StringId sid = SID(str);

    const auto iter = s_table.find(sid.GetValue());
    if (iter == s_table.end() || std::strcmp(str, iter->second.c_str()))
    {
      Add(str);
    }

    *pOut = sid;
    return true;
  }

}
