/*
-----------------------------------------------------------------------
Copyright 2012 iMinds-Vision Lab, University of Antwerp

Contact: astra@ua.ac.be
Website: http://astra.ua.ac.be


This file is part of the
All Scale Tomographic Reconstruction Antwerp Toolbox ("ASTRA Toolbox").

The ASTRA Toolbox is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

The ASTRA Toolbox is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with the ASTRA Toolbox. If not, see <http://www.gnu.org/licenses/>.

-----------------------------------------------------------------------
$Id$
*/

#ifndef _INC_ASTRA_MEX_HELPFUNCTIONS
#define _INC_ASTRA_MEX_HELPFUNCTIONS

#include <string>
#include <list>
#include <iostream>
#include <sstream>
#include <map>
#include <algorithm>
#include <mex.h>

#include <boost/lexical_cast.hpp>
#include <boost/any.hpp>

#include "astra/Globals.h"
#include "astra/Utilities.h"

#include "astra/ParallelProjectionGeometry2D.h"
#include "astra/FanFlatProjectionGeometry2D.h"
#include "astra/VolumeGeometry2D.h"

#include "astra/XMLDocument.h"
#include "astra/XMLNode.h"

std::string mex_util_get_string(const mxArray* pInput);
bool isOption(std::list<std::string> lOptions, std::string sOption);

bool mex_is_scalar(const mxArray* pInput);

std::map<std::string, mxArray*> parseStruct(const mxArray* pInput);
mxArray* buildStruct(std::map<std::string, mxArray*> mInput);
mxArray* vectorToMxArray(std::vector<astra::float32> mInput);

mxArray* anyToMxArray(boost::any _any);

astra::CProjectionGeometry2D* parseProjectionGeometryStruct(const mxArray*);
mxArray* createProjectionGeometryStruct(astra::CProjectionGeometry2D*);
astra::CVolumeGeometry2D* parseVolumeGeometryStruct(const mxArray*);
mxArray* createVolumeGeometryStruct(astra::CVolumeGeometry2D* _pReconGeom);

astra::XMLDocument* struct2XML(string rootname, const mxArray* pStruct);

mxArray* XML2struct(astra::XMLDocument* xml);
mxArray* XMLNode2struct(astra::XMLNode* xml);

void get3DMatrixDims(const mxArray* x, mwSize *dims);

#endif
