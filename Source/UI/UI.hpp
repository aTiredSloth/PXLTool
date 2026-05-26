#ifndef UI_HPP
#define UI_HPP
#include "SQLiteCpp/SQLiteCpp.h"
#include <SQLiteCpp/Database.h>

namespace UI
{
	int Start(SQLite::Database& pDB);
}

#endif
