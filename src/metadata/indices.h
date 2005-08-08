/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  Contributor(s):
*/

#ifndef FR_INDICES_H
#define FR_INDICES_H

#include <vector>
#include <string>
#include "metadataitem.h"
//------------------------------------------------------------------------------
class Index: public YxMetadataItem
{
public:
    virtual void accept(Visitor *v);

	enum IndexType { itAscending, itDescending };
	Index(bool unique, bool active, bool ascending, double statistics);

	bool isActive();
	bool isUnique();
	double getStatistics();
	IndexType getIndexType();
	std::string getFieldsAsString();
	std::vector<std::string> *getSegments();

private:
	bool uniqueFlagM;
	bool activeM;
	IndexType indexTypeM;
	double statisticsM;
	std::vector<std::string> segmentsM;
};
//------------------------------------------------------------------------------
#endif