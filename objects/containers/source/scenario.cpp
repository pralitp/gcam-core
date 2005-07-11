/*! 
* \file scenario.cpp
* \ingroup Objects
* \brief Scenario class source file.
* \author Sonny Kim
* \date $Date$
* \version $Revision$
*/				

#include "util/base/include/definitions.h"
#include <string>
#include <fstream>
#include <cassert>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>

#include "containers/include/scenario.h"
#include "util/base/include/model_time.h"
#include "marketplace/include/marketplace.h"
#include "containers/include/world.h"
#include "util/base/include/xml_helper.h"
#include "util/base/include/configuration.h"
#include "util/logger/include/ilogger.h"
#include "util/curves/include/curve.h"
#include "solution/solvers/include/solver.h"

using namespace std;
using namespace xercesc;

time_t ltime;
extern ofstream outFile;
const string Scenario::XML_NAME = "scenario";

//! Default construtor
/*! \todo Implement a factory method which chooses solvers to abstract this further. -JPL
*/
Scenario::Scenario() {
    runCompleted = false;
    marketplace.reset( new Marketplace() );

    // Get time and date before model run
    time( &ltime ); 
}

//! Destructor
Scenario::~Scenario() {
}

//! Return a reference to the modeltime->
const Modeltime* Scenario::getModeltime() const {
    return modeltime.get();
}

//! Return a constant reference to the goods and services marketplace.
const Marketplace* Scenario::getMarketplace() const {
    return marketplace.get();
}

//! Return a mutable reference to the goods and services marketplace.
Marketplace* Scenario::getMarketplace() {
    return marketplace.get();
}

//! Return a constant reference to the world object.
const World* Scenario::getWorld() const {
    return world.get();
}

//! Return a mutable reference to the world object.
World* Scenario::getWorld() {
    return world.get();
}

//! Set data members from XML input.
bool Scenario::XMLParse( const DOMNode* node ){
    // assume we were passed a valid node.
    assert( node );

    // set the scenario name.
    name = XMLHelper<string>::getAttrString( node, "name" );

    // get the children of the node.
    DOMNodeList* nodeList = node->getChildNodes();

    // loop through the children
    for ( int i = 0; i < static_cast<int>( nodeList->getLength() ); i++ ){
        DOMNode* curr = nodeList->item( i );
        string nodeName = XMLHelper<string>::safeTranscode( curr->getNodeName() );

        if( nodeName == "#text" ) {
            continue;
        }

        else if ( nodeName == "summary" ){
            scenarioSummary = XMLHelper<string>::getValueString( curr );
        }

		else if ( nodeName == Modeltime::getXMLNameStatic() ){
            if( !modeltime.get() ) {
                modeltime.reset( new Modeltime() );
                modeltime->XMLParse( curr );
                modeltime->set(); // This call cannot be delayed until completeInit() because it is needed first. 
            }
            else if ( Configuration::getInstance()->getBool( "debugChecking" ) ) { 
				ILogger& mainLog = ILogger::getLogger( "main_log" );
				mainLog.setLevel( ILogger::WARNING );
                mainLog << "Modeltime information cannot be modified in a scenario add-on." << endl;
            }
        }
		else if ( nodeName == World::getXMLNameStatic() ){
            if( !world.get() ) {
                world.reset( new World() );
            }
            world->XMLParse( curr );
        }
        else {
            ILogger& mainLog = ILogger::getLogger( "main_log" );
            mainLog.setLevel( ILogger::WARNING );
            mainLog << "Unrecognized text string: " << nodeName << " found while parsing scenario." << endl;
            return false;
        }
    } // end for loop
    return true;
}

//! Sets the name of the scenario. 
void Scenario::setName(string newName) {
    // Used to override the read-in scenario name.
    name = newName;
}

//! Finish all initializations needed before the model can run.
void Scenario::completeInit() {
    // Make sure that some name is set.
    if( name == "" ){
        ILogger& mainLog = ILogger::getLogger( "main_log" );
        mainLog.setLevel( ILogger::WARNING );
        mainLog << "No scenario name was set, using default." << endl;
        name = "NoScenarioName";
    }

    // Complete the init of the world object.
	if( world.get() ){
		world->completeInit();
	}
	else {
        ILogger& mainLog = ILogger::getLogger( "main_log" );
        mainLog.setLevel( ILogger::SEVERE );
        mainLog << "No world container was parsed from the input files." << endl;
	}

    // Create the solver and initialize with a pointer to the Marketplace and World.
    const string solverName = Configuration::getInstance()->getString( "SolverName" );
    solver = Solver::getSolver( solverName, marketplace.get(), world.get() );
    // Complete the init of the solution object.
    solver->init();
}

//! Write object to xml output stream.
void Scenario::toInputXML( ostream& out, Tabs* tabs ) const {
    
    // write heading for XML input file
    bool header = true;
    if (header) {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
        out << "<!-- edited with XMLSPY v5 rel. 2 U (http://www.xmlspy.com)";
        out << "by Son H. Kim (PNNL) -->" << endl;
        out << "<!--XML file generated by XMLSPY v5 rel. 2 U (http://www.xmlspy.com)-->" << endl;
    }

    string dateString = util::XMLCreateDate( ltime );
    out << "<" << XML_NAME << " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"";
    out << " xsi:noNamespaceSchemaLocation=\"C:\\PNNL\\Objects\\CVS\\Objects\\Objects.xsd\"";
    out << " name=\"" << name << "\" date=\"" << dateString << "\">" << endl;
    // increase the indent.
    tabs->increaseIndent();

    // summary notes on scenario
    tabs->writeTabs( out );
    out << "<summary>\"SRES B2 Scenario is used for this Reference Scenario\"</summary>" << endl;

    // write the xml for the class members.
    modeltime->toInputXML( out, tabs );
    world->toInputXML( out, tabs );
    // finished writing xml for the class members.

	XMLWriteClosingTag( XML_NAME, out, tabs );
}

//! Write out object to output stream for debugging.
void Scenario::toDebugXMLOpen( ostream& out, Tabs* tabs ) const {
    string dateString = util::XMLCreateDate( ltime );
    out << "<" << XML_NAME << " name=\"" << name << "\" date=\"" << dateString << "\">" << endl;

    tabs->increaseIndent();
    XMLWriteElement( "Debugging output", "summary", out, tabs );
}

//! Write out close scenario tag to output stream for debugging.
void Scenario::toDebugXMLClose( ostream& out, Tabs* tabs ) const {
    XMLWriteClosingTag( XML_NAME, out, tabs );
}

//! Return scenario name.
string Scenario::getName() const {
    return name; 
}

/*! \brief Run the scenario
* \param filenameEnding The string to add to the end of the debug output file for uniqueness.
* \return Whether all model runs solved successfully.
*/
bool Scenario::run( string filenameEnding ){

    Configuration* conf = Configuration::getInstance();
    ofstream xmlDebugStream;
    openDebugXMLFile( xmlDebugStream, filenameEnding );

    Tabs tabs;
    marketplace->initPrices(); // initialize prices
    toDebugXMLOpen( xmlDebugStream, &tabs );
    // sgm output file for debugging
    const string sgmOutFileName = conf->getFile( "ObjectSGMFileName", "ObjectSGMout.csv" );
    ofstream sgmOutFile;
	sgmOutFile.open( sgmOutFileName.c_str() ) ;
	util::checkIsOpen( sgmOutFile, sgmOutFileName );
    
    bool success = true;

    // Loop over time steps and operate model
    ILogger& mainLog = ILogger::getLogger( "main_log" );
    mainLog.setLevel( ILogger::NOTICE );
    ILogger& calibrationLog = ILogger::getLogger( "calibration_log" );
    calibrationLog.setLevel( ILogger::DEBUG );
    ILogger& worstMarketLog = ILogger::getLogger( "worst_market_log" );
    for( int per = 0; per < modeltime->getmaxper(); per++ ) {	
        // Write out some info.
        mainLog.setLevel( ILogger::NOTICE );
        mainLog << "Period " << per <<": "<< modeltime->getper_to_yr( per ) << endl;
        worstMarketLog.setLevel( ILogger::DEBUG );
        worstMarketLog << "Period " << per <<": "<< modeltime->getper_to_yr( per ) << endl;
        calibrationLog << "Period " << per <<": "<< modeltime->getper_to_yr( per ) << endl << endl;

        // Run the iteration of the model.
        marketplace->nullSuppliesAndDemands( per ); // initialize market demand to null
        marketplace->init_to_last( per ); // initialize to last period's info
        world->initCalc( per ); // call to initialize anything that won't change during calc
        // SGM Period 0 needs to clear out the supplies and demands put in by initCalc.
        if( per == 0 ){
            marketplace->nullSuppliesAndDemands( per );
        }
        world->calc( per ); // call to calculate initial supply and demand
        success &= solve( per ); // solution uses Bisect and NR routine to clear markets
        world->finalizePeriod( per );
        world->updateSummary( per ); // call to update summaries for reporting
        world->emiss_ind( per ); // call to calculate global emissions

        // Write out the results for debugging.
        world->toDebugXML( per, xmlDebugStream, &tabs );

		// SGM csv output
		csvSGMOutputFile( sgmOutFile, per );

        if( conf->getBool( "PrintDependencyGraphs" ) ) {
            printGraphs( per ); // Print out dependency graphs.
        }
        mainLog.setLevel( ILogger::NOTICE );
        mainLog << endl;
    }

    // Denote the run has been performed. 
    runCompleted = true;
    mainLog.setLevel( ILogger::NOTICE );
    mainLog << "Model run completed." << endl;

    // main output file for sgm, general results
    const string sgmGenFileName = Configuration::getInstance()->getFile( "ObjectSGMGenFileName", "ObjectSGMGen.csv" );
	ofstream sgmGenFile;
    sgmGenFile.open( sgmGenFileName.c_str() ) ;
	util::checkIsOpen( sgmGenFile, sgmGenFileName );
	// SGM csv general output, writes for all periods.
   	csvSGMGenFile( sgmGenFile, 0 );
    sgmGenFile.close();

    toDebugXMLClose( xmlDebugStream, &tabs ); // Close the xml debugging tag.

    // Run the climate model.
    world->runClimateModel();

    xmlDebugStream.close();
    sgmOutFile.close();
    return success;
}

/*! \brief A function which print dependency graphs showing fuel usage by sector.
*
* This function creates a filename and stream for printing the graph data in the dot graphing language.
* The filename is created from the dependencyGraphName configuration attribute concatenated with the period.
* The function then calls the World::printDependencyGraphs function to perform the printing.
* Once the data is printed, dot must be called to create the actual graph as follows:
* dot -Tpng depGraphs_8.dot -o graphs.png
* where depGraphs_8.dot is the file created by this function and graphs.png is the file you want to create.
* The output format can be changed, see the dot documentation for further information.
*
* \param period The period to print graphs for.
*/
void Scenario::printGraphs( const int period ) const {
    Configuration* conf = Configuration::getInstance();
    stringstream fileNameBuffer;
    // Create the filename.
    fileNameBuffer << conf->getFile( "dependencyGraphName", "graph" ) << "_" << period << ".dot";
    string fileName;
    fileNameBuffer >> fileName;
    
    ofstream graphStream;
    graphStream.open( fileName.c_str() );
    util::checkIsOpen( graphStream, fileName );

    world->printGraphs( graphStream, period );

    graphStream.close();
}

/*! \brief A function to generate a series of ghg emissions quantity curves based on an already performed model run.
* \details This function used the information stored in it to create a series of curves, one for each region,
* with each datapoint containing a time period and an amount of gas emissions.
* \note The user is responsible for deallocating the memory in the returned Curves.
* \author Josh Lurz
* \param ghgName The name of the ghg to create a set of curves for.
* \return A vector of Curve objects representing ghg emissions quantity by time period by region.
*/
const map<const string, const Curve*> Scenario::getEmissionsQuantityCurves( const string& ghgName ) const {
    /*! \pre The run has been completed. */
    return world->getEmissionsQuantityCurves( ghgName );
}

/*! \brief A function to generate a series of ghg emissions price curves based on an already performed model run.
* \details This function used the information stored in it to create a series of curves, one for each period,
* with each datapoint containing a time period and the price gas emissions. 
* \note The user is responsible for deallocating the memory in the returned Curves.
* \author Josh Lurz
* \param ghgName The name of the ghg to create a set of curves for.
* \return A vector of Curve objects representing the price of ghg emissions by time period by Region. 
*/
const map<const string,const Curve*> Scenario::getEmissionsPriceCurves( const string& ghgName ) const {
    /*! \pre The run has been completed. */
    return world->getEmissionsPriceCurves( ghgName );
}

/*! \brief Solve the marketplace using the Solver for a given period. 
*
* The solve method calls the solve method of the instance of the Solver object 
* that was created in the constructor. This method then checks for any errors that occurred while solving
* and reports the errors if it is the last period. 
* \return Whether all model periods solved successfully.
* \param period Period of the model to solve.
* \todo Fix the return codes. 
*/

bool Scenario::solve( const int period ){
    // Solve the marketplace. If the retcode is zero, add it to the unsolved periods. 
    if( !solver->solve( period ) ) {
        unsolvedPeriods.push_back( period );
    }
    // TODO: This should be added to the db. Using a logger would remove the dual writes.
    // If it was the last period print the ones that did not solve.
    if( modeltime->getmaxper() - 1 == period  ){
        ILogger& mainLog = ILogger::getLogger( "main_log" );
        mainLog.setLevel( ILogger::ERROR );
        if( static_cast<int>( unsolvedPeriods.size() ) == 0 ) {
            mainLog << "All model periods solved correctly." << endl;
            return true;
        }
        mainLog << "The following model periods did not solve: ";
        for( vector<int>::const_iterator i = unsolvedPeriods.begin(); i != unsolvedPeriods.end(); i++ ) {
            mainLog << *i << ", ";
        }
        mainLog << endl;
        return false;
    }
    return true; // The error will be sent after the last iteration.
}

//! Output Scenario members to a CSV file.
// I don't really like this function being hardcoded to an output file, but its very hardcoded.
void Scenario::csvOutputFile() const {
    // Open the output file.
    const Configuration* conf = Configuration::getInstance();
    const string outFileName = conf->getFile( "outFileName" );
    outFile.open( outFileName.c_str(), ios::out );
    util::checkIsOpen( outFile, outFileName ); 
    
    // Write results to the output file.
    // Minicam style output.
    outFile << "Region,Sector,Subsector,Technology,Variable,Units,";
    
    for ( int t = 0; t < modeltime->getmaxper(); t++ ) { 
        outFile << modeltime->getper_to_yr( t ) <<",";
    }
    outFile << "Date,Notes" << endl;

    // Write global market info to file
    marketplace->csvOutputFile( "global" );

    // Write world and regional info
    world->csvOutputFile();
    outFile.close();
}

//! Output Scenario members to the database.
void Scenario::dbOutput() const {
    world->dbOutput();
    marketplace->dbOutput();
}

//! Open the debugging XML file with the correct name and check for any errors.
void Scenario::openDebugXMLFile( ofstream& xmlDebugStream, const string& fileNameEnding ){
    // Need to insert the filename ending before the file type.
    const Configuration* conf = Configuration::getInstance();
    string debugFileName = conf->getFile( "xmlDebugFileName", "debug.xml" );
    size_t dotPos = debugFileName.find_last_of( "." );
    debugFileName = debugFileName.insert( dotPos, fileNameEnding );
    ILogger& mainLog = ILogger::getLogger( "main_log" );
    mainLog.setLevel( ILogger::DEBUG );
    mainLog << "Debugging information for this run in: " << debugFileName << endl;
    xmlDebugStream.open( debugFileName.c_str(), ios::out );
    util::checkIsOpen( xmlDebugStream, debugFileName );
}

/*! \brief Write SGM results to csv text file.
*/
void Scenario::csvSGMOutputFile( ostream& aFile, const int aPeriod ) {
	aFile <<  "**********************" << endl;
	aFile <<  "RESULTS FOR PERIOD:  " << aPeriod << endl;
	aFile <<  "**********************" << endl << endl;
	marketplace->csvSGMOutputFile( aFile, aPeriod );
	world->csvSGMOutputFile( aFile, aPeriod );
}

/*! \brief Write SGM general results for all periods to csv text file.
*/
void Scenario::csvSGMGenFile( ostream& aFile, const int aPeriod ) const {
    // Write out the file header.
	aFile << "SGM General Output " << endl;
	aFile << "Date & Time: " << ctime( &ltime ) << endl;

	world->csvSGMGenFile( aFile, aPeriod );
}
