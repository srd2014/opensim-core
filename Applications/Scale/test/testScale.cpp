// testIK.cpp
// Author: Ayman Habib based on Peter Loan's version
/* Copyright (c)  2005, Stanford University and Peter Loan.
* Use of the OpenSim software in source form is permitted provided that the following
* conditions are met:
* 	1. The software is used only for non-commercial research and education. It may not
*     be used in relation to any commercial activity.
* 	2. The software is not distributed or redistributed.  Software distribution is allowed 
*     only through https://simtk.org/home/opensim.
* 	3. Use of the OpenSim software or derivatives must be acknowledged in all publications,
*      presentations, or documents describing work in which OpenSim or derivatives are used.
* 	4. Credits to developers may not be removed from executables
*     created from modifications of the source.
* 	5. Modifications of source code must retain the above copyright notice, this list of
*     conditions and the following disclaimer. 
* 
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
*  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
*  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
*  SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
*  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
*  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR BUSINESS INTERRUPTION) OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
*  WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// INCLUDES
#include <string>
#include <OpenSim/version.h>
#include <OpenSim/Common/Storage.h>
#include <OpenSim/Common/IO.h>
#include <OpenSim/Common/ScaleSet.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Tools/ScaleTool.h>
#include <OpenSim/Common/MarkerData.h>
#include <OpenSim/Simulation/Model/MarkerSet.h>
#include <OpenSim/Simulation/Model/ForceSet.h>
#include <OpenSim/Common/LoadOpenSimLibrary.h>
#include <OpenSim/Simulation/Model/Analysis.h>
#include <OpenSim/Auxiliary/auxiliaryTestFunctions.h>

using namespace OpenSim;
using namespace std;

void scaleGait2354();
void scaleGait2354_GUI(bool useMarkerPlacement);

int main()
{
    try {
 
		scaleGait2354();
		scaleGait2354_GUI(false);
		//scaleGait2354_GUI(true);

    }
    catch (const Exception& e) {
        e.print(cerr);
        return 1;
    }
    cout << "Done" << endl;
    return 0;
}

void compareModel(const Model& resultModel, const std::string& stdFileName, double tol)
{
	// Load std model and realize it to Velocity just in case
	Model* refModel = new Model(stdFileName);
	SimTK::State& sStd = refModel->initSystem();

	SimTK::State& s = resultModel.updMultibodySystem().updDefaultState();
	resultModel.updMultibodySystem().realize(s, SimTK::Stage::Velocity);

	ASSERT(sStd.getNQ()==s.getNQ());	
	// put them in same configuration
	sStd.updQ() = s.getQ();
	refModel->updMultibodySystem().realize(sStd, SimTK::Stage::Velocity);

	ASSERT(sStd.getNU()==s.getNU());	
	ASSERT(sStd.getNZ()==s.getNZ());	

	// Now cycle thru ModelComponents recursively

	delete refModel;
}

void scaleGait2354()
{
	// SET OUTPUT FORMATTING
	IO::SetDigitsPad(4);

	std::string setupFilePath;
	ScaleTool* subject;
	Model* model;

	// Remove old results if any
	FILE* file2Remove = IO::OpenFile(setupFilePath+"subject01_scaleSet_applied.xml", "w");
	fclose(file2Remove);
	file2Remove = IO::OpenFile(setupFilePath+"subject01_simbody.osim", "w");
	fclose(file2Remove);

	// Construct model and read parameters file
	subject = new ScaleTool("subject01_Setup_Scale.xml");

	// Keep track of the folder containing setup file, wil be used to locate results to comapre against
	setupFilePath=subject->getPathToSubject();

	model = subject->createModel();

    SimTK::State& s = model->updMultibodySystem().updDefaultState();
    model->updMultibodySystem().realize(s, SimTK::Stage::Position );

	if(!model) {
		//throw Exception("scale: ERROR- No model specified.",__FILE__,__LINE__);
		cout << "scale: ERROR- No model specified.";
    }

	ASSERT(!subject->isDefaultModelScaler() && subject->getModelScaler().getApply());
	ModelScaler& scaler = subject->getModelScaler();
	ASSERT(scaler.processModel(s, model, subject->getPathToSubject(), subject->getSubjectMass()));

	/*
	if (!subject->isDefaultMarkerPlacer() && subject->getMarkerPlacer().getApply()) {
		MarkerPlacer& placer = subject->getMarkerPlacer();
	    if( false == placer.processModel(s, model, subject->getPathToSubject())) return(false);
	}
	else {
        return(1);
	}
	*/
	// Compare ScaleSet
	ScaleSet stdScaleSet = ScaleSet(setupFilePath+"std_subject01_scaleSet_applied.xml");

	const ScaleSet& computedScaleSet = ScaleSet(setupFilePath+"subject01_scaleSet_applied.xml");

	ASSERT(computedScaleSet == stdScaleSet);
    
	delete model;
	delete subject;
}

void scaleGait2354_GUI(bool useMarkerPlacement)
{
	// SET OUTPUT FORMATTING
	IO::SetDigitsPad(4);

	// Construct model and read parameters file
	ScaleTool* subject = new ScaleTool("subject01_Setup_Scale_GUI.xml");
	std::string setupFilePath=subject->getPathToSubject();

	// Remove old results if any
	FILE* file2Remove = IO::OpenFile(setupFilePath+"subject01_scaleSet_applied_GUI.xml", "w");
	fclose(file2Remove);
	file2Remove = IO::OpenFile(setupFilePath+"subject01_scaledOnly_GUI.osim", "w");
	fclose(file2Remove);

	Model guiModel("gait2354_simbody.osim");
	
	// Keep track of the folder containing setup file, wil be used to locate results to comapre against
	guiModel.initSystem();
	MarkerSet *markerSet = new MarkerSet(setupFilePath + subject->getGenericModelMaker().getMarkerSetFileName());
	guiModel.updateMarkerSet(*markerSet);

	// processedModelContext.processModelScale(scaleTool.getModelScaler(), processedModel, "", scaleTool.getSubjectMass())
	guiModel.getMultibodySystem().realizeTopology();
	SimTK::State* configState=&guiModel.updMultibodySystem().updDefaultState();
	bool retValue= subject->getModelScaler().processModel(*configState, &guiModel, setupFilePath, subject->getSubjectMass());
	// Model has changed need to recreate a valid state 
	guiModel.getMultibodySystem().realizeTopology();
    configState=&guiModel.updMultibodySystem().updDefaultState();
	guiModel.getMultibodySystem().realize(*configState, SimTK::Stage::Position);

	/*
	if (!subject->isDefaultMarkerPlacer() && subject->getMarkerPlacer().getApply()) {
		MarkerPlacer& placer = subject->getMarkerPlacer();
	    if( false == placer.processModel(s, model, subject->getPathToSubject())) return(false);
	}
	else {
        return(1);
	}
	*/
	// Compare ScaleSet
	ScaleSet stdScaleSet = ScaleSet(setupFilePath+"std_subject01_scaleSet_applied.xml");

	const ScaleSet& computedScaleSet = ScaleSet(setupFilePath+"subject01_scaleSet_applied_GUI.xml");

	ASSERT(computedScaleSet == stdScaleSet);

	delete subject;
}