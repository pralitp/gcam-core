/*! 
* \file gdp.cpp
* \ingroup Objects
* \brief The GDP class source file.
* \author Josh Lurz, Sonny Kim
* \date $Date$
* \version $Revision$
*/

#include "util/base/include/definitions.h"
#include <cmath>
#include <cassert>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>

#include "containers/include/gdp.h"
#include "demographics/include/demographic.h"
#include "containers/include/scenario.h"
#include "util/base/include/model_time.h"
#include "marketplace/include/marketplace.h"
#include "marketplace/include/imarket_type.h"
#include "util/base/include/xml_helper.h"
#include "util/logger/include/ilogger.h"

using namespace std;
using namespace xercesc;

extern Scenario* scenario;
// static initialize.
const string GDP::XML_NAME = "GDP";
const int BASE_PPP_YEAR = 1990;   // Base year for PPP conversion. PPP values are not known before about this time.
/*! \todo There are uninitialized memory reads all over this file. Lines: 308, 307, 285 (roughly) */
//! Default Constructor
GDP::GDP() {
	// Resize all vectors to the max period.
	const int maxper = scenario->getModeltime()->getmaxper();
	laborProdGrowthRate.resize( maxper );
	laborForceParticipationPercent.resize( maxper );
	laborForce.resize( maxper );
	gdpValue.resize( maxper );
	gdpPerCapita.resize( maxper );
	gdpValueAdjusted.resize( maxper );
	gdpPerCapitaAdjusted.resize( maxper );
	gdpPerCapitaAdjustedPPP.resize( maxper );
    gdpPerCapitaApproxPPP.resize( maxper );
	gdpAdjustedFlag.resize( maxper );
	calibrationGDPs.resize( maxper );
    gdpValueNotAdjusted.resize( maxper );
    gdpPerCapitaNotAdjusted.resize( maxper );
    baseGDP = 0;
	EnergyGDPElas = 0;
	PPPConversionFact = 1;
    PPPDelta = 0;
	constRatio = false;
    baseGDP = 0;
}

//! Destructor
GDP::~GDP(){
}

//! parses Population xml object
void GDP::XMLParse( const DOMNode* node ){
	// make sure we were passed a valid node.
	assert( node );

	DOMNodeList* nodeList = node->getChildNodes();
    const Modeltime* modeltime = scenario->getModeltime();

	for( unsigned int i = 0; i < nodeList->getLength(); ++i ){
		DOMNode* curr = nodeList->item( i );

		// get the name of the node.
		string nodeName = XMLHelper<string>::safeTranscode( curr->getNodeName() );

		if( nodeName == "#text" ) {
			continue;
		}
		// GDP to PPP conversion factor
        // Note that variable conversion attribute defaults to true
		else if ( nodeName == "PPPConvert" ){
		    PPPConversionFact = XMLHelper<double>::getValue( curr );
            constRatio = XMLHelper<bool>::getAttr( curr, "constRatio" );
		}
		// base-year GDP
		else if ( nodeName == "baseGDP" ){
			baseGDP = XMLHelper<double>::getValue( curr );
		}
		// Energy GDP elasticity. 
		else if ( nodeName == "e_GDP_elas" ){
			EnergyGDPElas = XMLHelper<double>::getValue( curr );
		}
		// labor force participation rate
		else if ( nodeName == "laborproductivity" ){
			XMLHelper<double>::insertValueIntoVector( curr, laborProdGrowthRate, modeltime );
		}
		// labor force participation rate
		else if( nodeName == "laborforce" ){
			XMLHelper<double>::insertValueIntoVector( curr, laborForceParticipationPercent, modeltime );
		} 
        else {
            ILogger& mainLog = ILogger::getLogger( "main_log" );
            mainLog.setLevel( ILogger::WARNING );
			mainLog << "Unrecognized text string: " << nodeName << " found while parsing GDP." << endl;
		}
	}
}

//! Writes datamembers to datastream in XML format.
void GDP::toInputXML( ostream& out, Tabs* tabs ) const {
	XMLWriteOpeningTag( getXMLNameStatic(), out, tabs );

	// GDP to PPP conversion factor
	XMLWriteElementAndAttribute( PPPConversionFact, "PPPConvert", out, tabs, constRatio, "constRatio" );

	// Write out base-year GDP
	XMLWriteElement( baseGDP, "baseGDP", out, tabs);

	// Write out gdp energy elasticity.
	XMLWriteElementCheckDefault( EnergyGDPElas, "e_GDP_elas", out, tabs, 0.0 );

	const Modeltime* modeltime = scenario->getModeltime();
	for( unsigned int iter = 0; iter < laborProdGrowthRate.size(); ++iter ){
		XMLWriteElement( laborProdGrowthRate[ iter ], "laborproductivity", out, tabs, modeltime->getper_to_yr( iter ) );
	}

	for( unsigned int iter = 0; iter < laborForceParticipationPercent.size(); ++iter ){
		XMLWriteElement( laborForceParticipationPercent[ iter ], "laborforce", out, tabs, modeltime->getper_to_yr( iter ) );
	}

    // Would want these in an xml-output file, but not in the input file.
    // write out MER-based GDP
    for( int m = 0; m < static_cast<int>( gdpValueAdjusted.size() ); m++ ){
        // XMLWriteElementCheckDefault( gdpValueAdjusted[ m ], "GDP(MER)", out, tabs, 0, modeltime->getper_to_yr( m ) );
    }

    // write out PPP-based GDP
    for( int m = 0; m < static_cast<int>( gdpValueAdjusted.size() ); m++ ){
        // XMLWriteElementCheckDefault( gdpValueAdjustedPPP[ m ], "GDP(PPP)", out, tabs, 0, modeltime->getper_to_yr( m ) );
    }

	XMLWriteClosingTag( getXMLNameStatic(), out, tabs );
}

//! Writes datamembers to debugging datastream in XML format.
void GDP::toDebugXML( const int period, ostream& out, Tabs* tabs ) const {

	const Modeltime* modeltime = scenario->getModeltime();

	XMLWriteOpeningTag( getXMLNameStatic(), out, tabs );

	// GDP to PPP conversion factor
	XMLWriteElementCheckDefault( PPPConversionFact, "PPPConvert", out, tabs, 0.0 );

	// Write out base-year GDP
	XMLWriteElement( baseGDP, "baseGDP", out, tabs);

	// Write out gdp energy elasticity.
	XMLWriteElementCheckDefault( EnergyGDPElas, "e_GDP_elas", out, tabs, 0.0 );

	XMLWriteElement( laborProdGrowthRate[ period ], "laborprod", out, tabs );

	XMLWriteElement( laborForceParticipationPercent[ period ], "laborforce_p", out, tabs );

	XMLWriteElement( laborForce[ period ], "laborforce", out, tabs );
	// Done writing XML for the class members.

	// write out MER-based GDP
    XMLWriteElementCheckDefault( gdpValueAdjusted[ period ], "GDP_MER", out, tabs, 0.0 );

	XMLWriteClosingTag( getXMLNameStatic(), out, tabs );
}

/*! \brief Get the XML node name in static form for comparison when parsing XML.
*
* This public function accesses the private constant string, XML_NAME.
* This way the tag is always consistent for both read-in and output and can be easily changed.
* The "==" operator that is used when parsing, required this second function to return static.
* \note A function cannot be static and virtual.
* \author Josh Lurz, James Blackwood
* \return The constant XML_NAME as a static.
*/
const std::string& GDP::getXMLNameStatic() {
	return XML_NAME;
}

/*! \brief Initialize labor force and the GDP without adjustements (needed for AgLU)
*
* This initialization function sets the labor force values and the time series for
* unadjusted GDP.

* \author Josh Lurz, Steve Smith
*/
//! Initialize the labor force.
void GDP::initData( const Demographic* regionalPop ){
    assert( regionalPop );
    const Modeltime* modeltime = scenario->getModeltime();
    for ( int i = 0; i < modeltime->getmaxper(); i++ ) {
        double population = regionalPop->getTotal( i );
        assert( population > 0 );
        assert( laborForceParticipationPercent[ i ] > 0 );
        laborForce[ i ] = population * laborForceParticipationPercent[ i ];
        assert( laborForce[ i ] > 0 );
        
        // Initialize the gdp.
        initialGDPcalc( i, population );
        gdpValueNotAdjusted[ i ] = getApproxGDP( i );
        gdpPerCapitaNotAdjusted[ i ] = gdpValueNotAdjusted[ i ] / population;
    }
}

//! Create calibration markets
void GDP::setupCalibrationMarkets( const string& regionName, const vector<double> aCalibrationGDPs ) {

	const string goodName = "GDP";
	const Modeltime* modeltime = scenario->getModeltime();
	Marketplace* marketplace = scenario->getMarketplace();

	if ( marketplace->createMarket( regionName, regionName, goodName, IMarketType::CALIBRATION ) ) {
		vector<double> tempLFPs( modeltime->getmaxper() );
		for( int i = 0; i < modeltime->getmaxper(); i++ ){
			tempLFPs[ i ] = pow( 1 + laborProdGrowthRate[ i ], modeltime->gettimestep( i ) );
		}
		marketplace->setPriceVector( goodName, regionName, tempLFPs );
	}

    // Set the constraint.
    for( int per = 1; per < modeltime->getmaxper(); per++ ){
        if( aCalibrationGDPs[ per ] > 0 ){
            marketplace->addToDemand( goodName, regionName, aCalibrationGDPs[ per  ], per );
            marketplace->setMarketToSolve( goodName, regionName, per );
        }
    }
    
    // Check for consistency with baseGDP attribute    
    const int basePer = modeltime->getyr_to_per( modeltime->getStartYear() );
    if ( aCalibrationGDPs[ basePer ] != 0 ) {
        if ( baseGDP != aCalibrationGDPs[ basePer ]  && baseGDP != 0 ) {
            ILogger& mainLog = ILogger::getLogger( "main_log" );
            mainLog.setLevel( ILogger::NOTICE );
            mainLog << "baseGDP overwritten with CalibrationGDPs value in " << regionName << endl;
        }
        baseGDP = aCalibrationGDPs[ basePer ];
    }
}

//! Write back the calibrated values from the marketplace to the member variables.
void GDP::writeBackCalibratedValues( const string& regionName, const int period ) {
	const Marketplace* marketplace = scenario->getMarketplace();
	const Modeltime* modeltime = scenario->getModeltime();
	const string goodName = "GDP";

	// Only need to write back calibrated values for the current period.
	double totalLaborProd = marketplace->getPrice( goodName, regionName, period );

	laborProdGrowthRate[ period ] = pow( totalLaborProd, double( 1 ) / double( modeltime->gettimestep( period ) ) ) - 1;

	// sjs -- put in check for illegal growth rate so that NaN does not occur
	if ( laborProdGrowthRate[ period ] <= -1 ) {
		cout << "ERROR: laborProd Growth Rate reset from " << laborProdGrowthRate[ period ]  << endl;
		laborProdGrowthRate[ period ]  = -0.99;
	}
}

//! Return the  total labor force productivity. 
double GDP::getTotalLaborProductivity( const int period ) const {
    assert( period >= 0 && period < scenario->getModeltime()->getmaxper() );
	const Modeltime* modeltime = scenario->getModeltime();
	return pow( 1 + laborProdGrowthRate[ period ], modeltime->gettimestep( period ) );
}

//! return the labor force (actual working)
double GDP::getLaborForce( const int per ) const {
    assert( per >= 0 && per < scenario->getModeltime()->getmaxper() );
	return laborForce[ per ];
}

//! Write GDP info to text file
void GDP::csvOutputFile( const string& regionName ) const {
   const Modeltime* modeltime = scenario->getModeltime();
   const int maxPeriod = modeltime->getmaxper();
   vector<double> temp( maxPeriod );

   // function protocol
   void fileoutput3( string var1name,string var2name,string var3name,
        string var4name,string var5name,string uname,vector<double> dout);

   // write gdp to temporary array since not all will be sent to output
   for ( int i = 0; i < maxPeriod; i++ ) {
        temp[ i ] = laborProdGrowthRate[ i ];
   }
   fileoutput3( regionName," "," "," ", "labor prod", "%/yr", temp );	
	
	// write gdp and adjusted gdp for region
	fileoutput3(regionName," "," "," ","GDP","Mil90US$",gdpValueAdjusted);
	fileoutput3(regionName," "," "," ","GDPperCap","thousand90US$",gdpPerCapitaAdjusted);
	fileoutput3(regionName," "," "," ","PPPperCap","thousand90US$",gdpPerCapitaAdjustedPPP);
}

//! MiniCAM output to file
void GDP::dbOutput( const string& regionName ) const {
	const Modeltime* modeltime = scenario->getModeltime();
	const int maxPeriod = modeltime->getmaxper();
	vector<double> temp( maxPeriod );

	// function protocol
	void dboutput4(string var1name,string var2name,string var3name,string var4name,
		string uname,vector<double> dout);

	// labor productivity
	for( int i = 0; i < maxPeriod; i++ ){
		temp[ i ] = laborProdGrowthRate[ i ];
	}
	dboutput4( regionName, "General", "LaborProd", "GrowthRate", "perYr", temp );	

	// write gdp and adjusted gdp for region
	dboutput4(regionName,"General","GDP90$","GDP(90mer)","Mil90US$", gdpValueAdjusted );
	dboutput4(regionName,"General","GDP90$","GDPApprox(90mer)","Mil90US$", gdpValue );
	dboutput4(regionName,"General","GDP","perCap","thousand90US$", gdpPerCapitaAdjusted );
	dboutput4(regionName,"General","GDP90$","perCAP_PPP","thousand90US$", gdpPerCapitaAdjustedPPP );
}

/*! Calculate innitial regional gdps.
*
*  Routine calcuates GDPs without current period energy adjustment.
*  The gdpValue and gdpPerCapita variables have values that are approximations to the current GDP,
*  these use the adjusted GDP value from the previous period, 
*  without adjusting for energy feedbacks in the current period
*  the "adjusted" values contain the values as adjusted for energy (and ultimate any other) feedbacks.
*
* \author Steve Smith, Sonny Kim, Josh Lurz(?),
* \param period Model time period
* \param population Population for this period to use to initialize the GDP object.
*/
void GDP::initialGDPcalc( const int period, const double population ) {

	const Modeltime* modeltime = scenario->getModeltime();
	const int basePer = modeltime->getBasePeriod();

	// Set flag, current GDP values are not adjusted
	gdpAdjustedFlag[ period ] = false; 	 
	if ( period <= modeltime->getyr_to_per( 1990 ) ) {
		gdpAdjustedFlag[ period ] = true; // GDP is never adjusted for historial periods
	}

	if ( period == basePer ) {
		gdpValue[ period ] = baseGDP; 
		gdpValueAdjusted[ period ] = gdpValue[ period ];
	}
	else {
		double currentLF = getLaborForce( period );
		double lastLF = getLaborForce( period - 1 );
		double tlab = getTotalLaborProductivity( period );
        // There is an unitialized read on the next line of a double.
		gdpValue[ period ] = gdpValueAdjusted[ period - 1 ] * tlab * ( currentLF / lastLF );
		gdpValueAdjusted[ period ] = gdpValue[ period ]; // Temporary value so that is never zero
		if ( gdpValue[period] == 0 ) {
            cerr << "Error: GDP = 0 in initial calculation.  Current Labor Force: " << currentLF
                << "  Last Labor Force: " << lastLF << "  lab: " << tlab << endl;
		}
	}

	// gdp period capita 
	// gdpValue is in millions, population in 1000's, so result is in 1000's of dollars per capita
    gdpPerCapita[ period ] = gdpValue[ period ] / population;   
   
   // Temporary values so that if requested a real value is returned (with error warning)
    gdpPerCapitaAdjusted[ period ] = gdpPerCapita[ period ]; 
    gdpPerCapitaAdjustedPPP[ period ] = gdpValue[ period ] / population;
    
    // Determine approximate PPP-based GDP per capita
    gdpPerCapitaApproxPPP[ period ] = calculatePPPPerCap( period, gdpPerCapita[ period ] );

}

/*! Adjust regional gdp for energy service price effect
*  Also calculates PPP-based GDP per capita
*  Note that GDP is only adjusted for periods after 1990. See notes for method initialGDPcalc().
* 
* \author Steve Smith, Sonny Kim, Josh Lurz(?),
* \param period Model time period
* \param priceRatio Energy service price ratio
*/
void GDP::adjustGDP( const int period, const double priceRatio ) {
    const Modeltime* modeltime = scenario->getModeltime();

    if ( period > modeltime->getyr_to_per(1990) ) {
        // adjust gdp using energy cost changes and energy to gdp feedback elasticity
        gdpValueAdjusted[ period ] = gdpValue[ period ]*pow( priceRatio, EnergyGDPElas );
        if ( !util::isValidNumber( gdpValueAdjusted[ period ] ) ) {
            ILogger& mainLog = ILogger::getLogger( "main_log" );
            mainLog.setLevel( ILogger::ERROR );
            mainLog << "Error calculating gdpAdj in gdp.adjustGDP(). " << endl;

            // Reset value so as not to propogate error further.
            gdpValueAdjusted[ period ]  = gdpValue[ period ];
        }
        gdpPerCapitaAdjusted[ period ] = gdpPerCapita[ period ] * gdpValueAdjusted[ period ] / gdpValue[ period ];
        gdpAdjustedFlag[ period ] = true;
    }
    gdpPerCapitaAdjustedPPP[ period ] = calculatePPPPerCap( period, gdpPerCapitaAdjusted[ period ] );
}

/*! Calculate GDP on a PPP basis
* 
* This uses the conversion factor from getPPPMERRatio to convert Market Exchange Rate basis GDP values into PPP values
*
* \author Steve Smith
* \param period Model time period
* \param marketGDPperCapAdj Adjusted GDP per capita (market basis)
*/
double GDP::calculatePPPPerCap( const int period, const double marketGDPperCap ) {
    
    return getPPPMERRatio( period, marketGDPperCap ) * marketGDPperCap;

}


/*! Return the ratio of PPP to MER GDP
* 
* This routine performs a logarithmic conversion between market exchange rate (MER) and PPP based GDP. 
* This conversion simulates the process of a developing economy transforming to a market economy where
* all sectors participate. This is, therefore, meant to account for the large MER/PPP differences between
* developing and market economies, not the fairly small differences between OECD market economies.
* In the base year (1990), values start at the PPP/MER ratio given as input data (taken from world bank or other
* studies). PPP and MER GDP values then converge exponentially until the crossover point is reached, 
* after which values are equal.  
* See Smith et al. (2004), "Future SO2 Emissions" paper for a full description of the conversion.
*
* NOTE, this routine uses the value passed in for "marketGDPperCap" to calculate the conversion. 
* In this way the same routine can be used to calculate both the approximate PPP and the exact PPP
* 
* \author Steve Smith
* \param period Model time period
*/
double GDP::getPPPMERRatio( const int period, const double marketGDPperCap ) {
    const Modeltime* modeltime = scenario->getModeltime();
	const double CROSSOVER_POINT = 15.0; // Point at which PPP and Market values are equal ($15,000 per capita)
 
    double conversionFactor;
    
	// Don't do variable conversion if turned off for this region or if is before conversion data exist (before 1990)
   // Also don't do conversion is PPPConversionFact is < 1 since this is not defined!
	if ( constRatio || ( period < modeltime->getyr_to_per( BASE_PPP_YEAR ) || ( PPPConversionFact < 1 ) ) ) {
		conversionFactor = PPPConversionFact;
	} else {
		// Only calculate this parameter in 1990 since won't change after that
		if  ( period == modeltime->getyr_to_per( BASE_PPP_YEAR ) )  {
			try { 
				PPPDelta = log( PPPConversionFact ) / log( marketGDPperCap / CROSSOVER_POINT );
			} catch(...) {
				cerr << "Error calculating PPPDelta. "<< "PPPConversionFact = "<< PPPConversionFact;
				cerr << "; marketGDPperCap = "<< marketGDPperCap << endl;
			}
		}

		// Now do the conversion. 
		if ( marketGDPperCap > CROSSOVER_POINT ) { // If GDP/cap is > crossover point then set equal.
			conversionFactor = 1.0;
		} else {
         try {
            conversionFactor =  pow( marketGDPperCap / CROSSOVER_POINT , PPPDelta );
         } catch(...) {
            conversionFactor = 1.0;
            cerr << "Error calculating PPP basis GDP in gdp.calculatePPPPerCap()" << endl;
         }
      }
	}
    
    return conversionFactor;
}

/*! Return approximate GDP per capita scaled to base year
* 
* This routine should be used only in the case where GDP per capita is needed before energy prices are available.
*
* \author Steve Smith
* \param period Model time period
*/
double GDP::getApproxScaledGDPperCap( const int period ) const {
	const Modeltime* modeltime = scenario->getModeltime();
    assert( gdpPerCapita[ modeltime->getBasePeriod() ] );
	return gdpPerCapita[ period ] / gdpPerCapita[ modeltime->getBasePeriod() ];
}

/*! Return approximate PPP per capita
* 
* This routine should be used only in the case where PPP-based GDP per capita is needed before energy prices are available.
*
* \author Steve Smith
* \param period Model time period
*/
double GDP::getApproxPPPperCap( const int period ) const {

	return gdpPerCapitaApproxPPP[ period ];
}

/*! Return approximate GDP scaled to base year
* 
* This routine should be used only in the case where GDP is needed before energy prices are available.
*
* \author Sonny Kim
* \param period Model time period
*/
double GDP::getApproxScaledGDP( const int period ) const {
	const Modeltime* modeltime = scenario->getModeltime();
    assert( gdpValue[ modeltime->getBasePeriod() ] );
	return gdpValue[ period ] / gdpValue[ modeltime->getBasePeriod() ];
}

/*! Return approximate GDP per capita (1000's of dollars/cap)
* 
* This routine should be used only in the case where GDP per cap is needed before energy prices are available.
*
* \author Steve Smith
* \param period Model time period
*/
double GDP::getApproxGDPperCap( const int period ) const {
	return gdpPerCapita[ period ];
}

/*! Return adjusted GDP scaled to base year
* 
* This routine should be used in preference to getApproxScaledGDPperCap() above
*
* \author Steve Smith
* \param period Model time period
*/
double GDP::getScaledGDPperCap( const int period ) const {
	if ( !gdpAdjustedFlag[ period ] ) {
		cerr << "ERROR - Request for adjusted GDP -- not calculated yet" << endl;
	}
	const Modeltime* modeltime = scenario->getModeltime();
    assert( gdpPerCapitaAdjusted[ modeltime->getBasePeriod() ] > 0 );
	return gdpPerCapitaAdjusted[ period ] / gdpPerCapitaAdjusted[ modeltime->getBasePeriod() ];
}

/*! Return GDP per capita (in $1000's of dollars)
*
* \author Steve Smith
* \param period Model time period
*/
double GDP::getGDPperCap( const int period ) const {
	if ( !gdpAdjustedFlag[ period ] ) {
		cerr << "ERROR - Request for adjusted GDP -- not calculated yet" << endl;
	}
	return gdpPerCapitaAdjusted[ period ];
}

/*! Return approximate GDP (in $1000's of dollars, before scaling)
*
* \author Steve Smith
* \param period Model time period
*/
double GDP::getApproxGDP( const int period ) const {
	return gdpValue[ period ];
}

/*! Return GDP without any energy price adjustments for any period (in $1000's of dollars)
*
* This function is meant to be used by AgLU and any other routine that needs a stable GDP for future periods
*
* \author Steve Smith
* \param period Model time period
*/
double GDP::getGDPNotAdjusted( const int period ) const {
	return gdpValueNotAdjusted[ period ];
}

/*! Return GDP per capita without any energy price adjustments for any period (in $1000's of dollars)
*
* This function is meant to be used by AgLU and any other routine that needs a stable GDP for future periods
*
* \author Steve Smith
* \param period Model time period
*/
double GDP::getGDPPerCapitaNotAdjusted( const int period ) const {
	return gdpPerCapitaNotAdjusted[ period ];
}

/*! Return PPP-based GDP per capita (in $1000's of dollars)
* 
* \author Steve Smith
* \param period Model time period
*/
double GDP::getPPPGDPperCap( const int period ) const {
    if ( !gdpAdjustedFlag[ period ] ) {
        cerr << "ERROR - Request for adjusted GDP -- not calculated yet" << endl;
    }
    return gdpPerCapitaAdjustedPPP[ period ] ;
}

/*! Return MER-based GDP in constant dollars
* 
* \author Steve Smith
* \param period Model time period
*/
double GDP::getGDP( const int period ) const {
	if ( !gdpAdjustedFlag[ period ] ) {
		cerr << "ERROR - Request for adjusted GDP -- not calculated yet" << endl;
	}
	return gdpValueAdjusted[ period ] ;
}

/*! Return either approximate GDP or adjusted GDP scaled to base year
* 
* This routine is used where the model doesn't know if the scaled GDP has been calculated yet. 
* Should be used sparingly -- is intended to be used in subsector and technology share calculations 
* where it is not necessarilly known if the adjusted GDP is available.
*
* \author Steve Smith
* \param period Model time period
*/
double GDP::getBestScaledGDPperCap( const int period ) const {
	if ( !gdpAdjustedFlag[ period ] ) {
		return getApproxScaledGDPperCap( period );
	}
    return getScaledGDPperCap( period );
}

