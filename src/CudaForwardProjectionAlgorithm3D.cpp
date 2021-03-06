/*
-----------------------------------------------------------------------
Copyright: 2010-2014, iMinds-Vision Lab, University of Antwerp
                2014, CWI, Amsterdam

Contact: astra@uantwerpen.be
Website: http://sf.net/projects/astra-toolbox

This file is part of the ASTRA Toolbox.


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

#include "astra/CudaForwardProjectionAlgorithm3D.h"

#ifdef ASTRA_CUDA

#include <boost/lexical_cast.hpp>

#include "astra/AstraObjectManager.h"

#include "astra/CudaProjector3D.h"
#include "astra/ConeProjectionGeometry3D.h"
#include "astra/ParallelProjectionGeometry3D.h"
#include "astra/ParallelVecProjectionGeometry3D.h"
#include "astra/ConeVecProjectionGeometry3D.h"

#include "../cuda/3d/astra3d.h"

using namespace std;

namespace astra {

// type of the algorithm, needed to register with CAlgorithmFactory
std::string CCudaForwardProjectionAlgorithm3D::type = "FP3D_CUDA";

//----------------------------------------------------------------------------------------
// Constructor
CCudaForwardProjectionAlgorithm3D::CCudaForwardProjectionAlgorithm3D() 
{
	m_bIsInitialized = false;
	m_iGPUIndex = -1;
	m_iDetectorSuperSampling = 1;
	m_pProjector = 0;
	m_pProjections = 0;
	m_pVolume = 0;

}

//----------------------------------------------------------------------------------------
// Destructor
CCudaForwardProjectionAlgorithm3D::~CCudaForwardProjectionAlgorithm3D() 
{

}

//---------------------------------------------------------------------------------------
// Initialize - Config
bool CCudaForwardProjectionAlgorithm3D::initialize(const Config& _cfg)
{
	ASTRA_ASSERT(_cfg.self);
	ConfigStackCheck<CAlgorithm> CC("CudaForwardProjectionAlgorithm3D", this, _cfg);	

	XMLNode* node;
	int id;

	// sinogram data
	node = _cfg.self->getSingleNode("ProjectionDataId");
	ASTRA_CONFIG_CHECK(node, "CudaForwardProjection3D", "No ProjectionDataId tag specified.");
	id = boost::lexical_cast<int>(node->getContent());
	m_pProjections = dynamic_cast<CFloat32ProjectionData3DMemory*>(CData3DManager::getSingleton().get(id));
	ASTRA_DELETE(node);
	CC.markNodeParsed("ProjectionDataId");

	// reconstruction data
	node = _cfg.self->getSingleNode("VolumeDataId");
	ASTRA_CONFIG_CHECK(node, "CudaForwardProjection3D", "No VolumeDataId tag specified.");
	id = boost::lexical_cast<int>(node->getContent());
	m_pVolume = dynamic_cast<CFloat32VolumeData3DMemory*>(CData3DManager::getSingleton().get(id));
	ASTRA_DELETE(node);
	CC.markNodeParsed("VolumeDataId");

	// optional: projector
	node = _cfg.self->getSingleNode("ProjectorId");
	if (node) {
		id = boost::lexical_cast<int>(node->getContent());
		m_pProjector = CProjector3DManager::getSingleton().get(id);
		ASTRA_DELETE(node);
	} else {
		m_pProjector = 0; // TODO: or manually construct default projector?
	}
	CC.markNodeParsed("ProjectorId");

	// GPU number
	m_iGPUIndex = (int)_cfg.self->getOptionNumerical("GPUindex", -1);
	CC.markOptionParsed("GPUindex");
	m_iDetectorSuperSampling = (int)_cfg.self->getOptionNumerical("DetectorSuperSampling", 1);
	CC.markOptionParsed("DetectorSuperSampling");

	// success
	m_bIsInitialized = check();

	if (!m_bIsInitialized)
		return false;

	return true;	
}


bool CCudaForwardProjectionAlgorithm3D::initialize(CProjector3D* _pProjector, 
                                  CFloat32ProjectionData3DMemory* _pProjections, 
                                  CFloat32VolumeData3DMemory* _pVolume,
                                  int _iGPUindex, int _iDetectorSuperSampling)
{
	m_pProjector = _pProjector;
	
	// required classes
	m_pProjections = _pProjections;
	m_pVolume = _pVolume;

	m_iDetectorSuperSampling = _iDetectorSuperSampling;
	m_iGPUIndex = _iGPUindex;

	// success
	m_bIsInitialized = check();

	if (!m_bIsInitialized)
		return false;

	return true;
}

//----------------------------------------------------------------------------------------
// Check
bool CCudaForwardProjectionAlgorithm3D::check() 
{
	// check pointers
	//ASTRA_CONFIG_CHECK(m_pProjector, "Reconstruction2D", "Invalid Projector Object.");
	ASTRA_CONFIG_CHECK(m_pProjections, "FP3D_CUDA", "Invalid Projection Data Object.");
	ASTRA_CONFIG_CHECK(m_pVolume, "FP3D_CUDA", "Invalid Volume Data Object.");

	// check initializations
	//ASTRA_CONFIG_CHECK(m_pProjector->isInitialized(), "Reconstruction2D", "Projector Object Not Initialized.");
	ASTRA_CONFIG_CHECK(m_pProjections->isInitialized(), "FP3D_CUDA", "Projection Data Object Not Initialized.");
	ASTRA_CONFIG_CHECK(m_pVolume->isInitialized(), "FP3D_CUDA", "Volume Data Object Not Initialized.");

	ASTRA_CONFIG_CHECK(m_iDetectorSuperSampling >= 1, "FP3D_CUDA", "DetectorSuperSampling must be a positive integer.");
	ASTRA_CONFIG_CHECK(m_iGPUIndex >= -1, "FP3D_CUDA", "GPUIndex must be a non-negative integer.");

	// check compatibility between projector and data classes
//	ASTRA_CONFIG_CHECK(m_pSinogram->getGeometry()->isEqual(m_pProjector->getProjectionGeometry()), "SIRT_CUDA", "Projection Data not compatible with the specified Projector.");
//	ASTRA_CONFIG_CHECK(m_pReconstruction->getGeometry()->isEqual(m_pProjector->getVolumeGeometry()), "SIRT_CUDA", "Reconstruction Data not compatible with the specified Projector.");

	// todo: turn some of these back on

// 	ASTRA_CONFIG_CHECK(m_pProjectionGeometry, "SIRT_CUDA", "ProjectionGeometry not specified.");
// 	ASTRA_CONFIG_CHECK(m_pProjectionGeometry->isInitialized(), "SIRT_CUDA", "ProjectionGeometry not initialized.");
// 	ASTRA_CONFIG_CHECK(m_pReconstructionGeometry, "SIRT_CUDA", "ReconstructionGeometry not specified.");
// 	ASTRA_CONFIG_CHECK(m_pReconstructionGeometry->isInitialized(), "SIRT_CUDA", "ReconstructionGeometry not initialized.");

	// check dimensions
	//ASTRA_CONFIG_CHECK(m_pSinogram->getAngleCount() == m_pProjectionGeometry->getProjectionAngleCount(), "SIRT_CUDA", "Sinogram data object size mismatch.");
	//ASTRA_CONFIG_CHECK(m_pSinogram->getDetectorCount() == m_pProjectionGeometry->getDetectorCount(), "SIRT_CUDA", "Sinogram data object size mismatch.");
	//ASTRA_CONFIG_CHECK(m_pReconstruction->getWidth() == m_pReconstructionGeometry->getGridColCount(), "SIRT_CUDA", "Reconstruction data object size mismatch.");
	//ASTRA_CONFIG_CHECK(m_pReconstruction->getHeight() == m_pReconstructionGeometry->getGridRowCount(), "SIRT_CUDA", "Reconstruction data object size mismatch.");
	
	// check restrictions
	// TODO: check restrictions built into cuda code

	// success
	m_bIsInitialized = true;
	return true;
}


void CCudaForwardProjectionAlgorithm3D::setGPUIndex(int _iGPUIndex)
{
	m_iGPUIndex = _iGPUIndex;
}

//---------------------------------------------------------------------------------------
// Information - All
map<string,boost::any> CCudaForwardProjectionAlgorithm3D::getInformation()
{
	map<string,boost::any> res;
	res["ProjectionGeometry"] = getInformation("ProjectionGeometry");
	res["VolumeGeometry"] = getInformation("VolumeGeometry");
	res["ProjectionDataId"] = getInformation("ProjectionDataId");
	res["VolumeDataId"] = getInformation("VolumeDataId");
	res["GPUindex"] = getInformation("GPUindex");
	res["GPUindex"] = getInformation("GPUindex");
	res["DetectorSuperSampling"] = getInformation("DetectorSuperSampling");
	return mergeMap<string,boost::any>(CAlgorithm::getInformation(), res);
}

//---------------------------------------------------------------------------------------
// Information - Specific
boost::any CCudaForwardProjectionAlgorithm3D::getInformation(std::string _sIdentifier)
{
	// TODO: store these so we can return them?
	if (_sIdentifier == "ProjectionGeometry")	{ return string("not implemented"); }
	if (_sIdentifier == "VolumeGeometry")	{ return string("not implemented"); }
	if (_sIdentifier == "GPUindex")	{ return m_iGPUIndex; }
	if (_sIdentifier == "DetectorSuperSampling")	{ return m_iDetectorSuperSampling; }

	if (_sIdentifier == "ProjectionDataId") {
		int iIndex = CData3DManager::getSingleton().getIndex(m_pProjections);
		if (iIndex != 0) return iIndex;
		return std::string("not in manager");
	}
	if (_sIdentifier == "VolumeDataId") {
		int iIndex = CData3DManager::getSingleton().getIndex(m_pVolume);
		if (iIndex != 0) return iIndex;
		return std::string("not in manager");
	}
	return CAlgorithm::getInformation(_sIdentifier);
}

//----------------------------------------------------------------------------------------
// Run
void CCudaForwardProjectionAlgorithm3D::run(int)
{
	// check initialized
	assert(m_bIsInitialized);

	const CProjectionGeometry3D* projgeom = m_pProjections->getGeometry();
	const CConeProjectionGeometry3D* conegeom = dynamic_cast<const CConeProjectionGeometry3D*>(projgeom);
	const CParallelProjectionGeometry3D* par3dgeom = dynamic_cast<const CParallelProjectionGeometry3D*>(projgeom);
	const CConeVecProjectionGeometry3D* conevecgeom = dynamic_cast<const CConeVecProjectionGeometry3D*>(projgeom);
	const CParallelVecProjectionGeometry3D* parvec3dgeom = dynamic_cast<const CParallelVecProjectionGeometry3D*>(projgeom);
	const CVolumeGeometry3D& volgeom = *m_pVolume->getGeometry();

	Cuda3DProjectionKernel projKernel = ker3d_default;
	if (m_pProjector) {
		CCudaProjector3D* projector = dynamic_cast<CCudaProjector3D*>(m_pProjector);
		projKernel = projector->getProjectionKernel();
	}

	if (conegeom) {
		astraCudaConeFP(m_pVolume->getDataConst(), m_pProjections->getData(),
		                volgeom.getGridColCount(),
		                volgeom.getGridRowCount(),
		                volgeom.getGridSliceCount(),
		                conegeom->getProjectionCount(),
		                conegeom->getDetectorColCount(),
		                conegeom->getDetectorRowCount(),
		                conegeom->getOriginSourceDistance(),
		                conegeom->getOriginDetectorDistance(),
		                conegeom->getDetectorSpacingX(),
		                conegeom->getDetectorSpacingY(),
		                conegeom->getProjectionAngles(),
		                m_iGPUIndex, m_iDetectorSuperSampling);
	} else if (par3dgeom) {
		astraCudaPar3DFP(m_pVolume->getDataConst(), m_pProjections->getData(),
		                 volgeom.getGridColCount(),
		                 volgeom.getGridRowCount(),
		                 volgeom.getGridSliceCount(),
		                 par3dgeom->getProjectionCount(),
		                 par3dgeom->getDetectorColCount(),
		                 par3dgeom->getDetectorRowCount(),
		                 par3dgeom->getDetectorSpacingX(),
		                 par3dgeom->getDetectorSpacingY(),
		                 par3dgeom->getProjectionAngles(),
		                 m_iGPUIndex, m_iDetectorSuperSampling,
		                 projKernel);
	} else if (parvec3dgeom) {
		astraCudaPar3DFP(m_pVolume->getDataConst(), m_pProjections->getData(),
		                 volgeom.getGridColCount(),
		                 volgeom.getGridRowCount(),
		                 volgeom.getGridSliceCount(),
		                 parvec3dgeom->getProjectionCount(),
		                 parvec3dgeom->getDetectorColCount(),
		                 parvec3dgeom->getDetectorRowCount(),
		                 parvec3dgeom->getProjectionVectors(),
		                 m_iGPUIndex, m_iDetectorSuperSampling,
		                 projKernel);
	} else if (conevecgeom) {
		astraCudaConeFP(m_pVolume->getDataConst(), m_pProjections->getData(),
		                volgeom.getGridColCount(),
		                volgeom.getGridRowCount(),
		                volgeom.getGridSliceCount(),
		                conevecgeom->getProjectionCount(),
		                conevecgeom->getDetectorColCount(),
		                conevecgeom->getDetectorRowCount(),
		                conevecgeom->getProjectionVectors(),
		                m_iGPUIndex, m_iDetectorSuperSampling);
	} else {
		ASTRA_ASSERT(false);
	}

}


}

#endif
