
/*
 * Optimization Checker Algorithm 2017
 * This script is built to check user’s solution compatibility and validity.
 * This main script is composed of the following parts:
 *  - main() function called by default when program is executed.
 *  - Test Tode Tenu function.
 *  - StatisticsLog create function.
 *  - Display plate area usage function.
 *  - Constraints verification functions.
 *  - CSV files parse functions.
 *  - Tree structure build functions.
 *  - Classes instantiation functions.
**/

#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

#include "main.h"
#include "GlassNode.h"
#include "GlassPlate.h"
#include "GlassItem.h"
#include "GlassStack.h"

using namespace std;

unsigned int active_log = 1; // Once log file is generated this var will be cleared.
ofstream log_file; // Reference to log file.

ofstream statistics_file; // Reference to statisticsLog file.
bool opened_file = true; // Boolean to test if one of needed files (batch, solution, defects and optimization parameters) was not opened.

string file_idx; // To save used file index (Normal mode).
string testPath; // To save path according to chosen test.
string instance; // To save used file index (Test mode).

unsigned int s_idx = 0; // Solution index, used precise where to insert solution node in sol_items array.
unsigned int plates_nbr = 0; // Save solution used plates number.
unsigned int stack_nbr = 0; // Save batch stack number.
unsigned int plates_list [PLATES_NBR_LIMIT][2];   // Save number of nodes for each plate used in user solution.
                                                  // plates_list[X][Y]:   X dimension contains plates Ids list
                                                  //                      Y dimension contains number of nodes of plate X
unsigned int lines_nbr = 0; // csv file lines number.
unsigned int node_nbr = 0; // Solution file nodes number.
unsigned int useful_node = 0; // Solution useful nodes number (node not a branch or waste or residual).
unsigned int branch_node = 0; // Solution branch nodes number
unsigned int residual_node = 0; // Solution residual nodes number.
unsigned int waste_node = 0; // Solution waste nodes number.
unsigned int batch_items = 0; // Batch items number.
unsigned int constraint_error = 0; // Success constraint, combination of solution's occured errors (refer to main.h to find errors list).
unsigned int defects_nbr = 0; // Defects file defects number.

unsigned int total_waste = 0; // Sum of wasted area.
unsigned int total_useful = 0; // Sum of used area.

int max_cut_stage = 0; // Solution max used cut stage.

unsigned int plate_nbr_limit = 0; // To save Optimization parameters plate number limit constraint.
unsigned int plate_w = 0; // To save Optimization parameters plate's length constraint.
unsigned int plate_h = 0; // To save Optimization parameters plate's width constraint.
unsigned int min1Cut = 0; // To save Optimization parameters cut 1 & 2 min1Cut constraint.
unsigned int min2Cut = 0; // To save Optimization parameters cut 1 & 2 min2Cut constraint.
unsigned int max1Cut = 0; // To save Optimization parameters cut 1 & 2 max1Cut  constraint.
unsigned int waste_min = 0; // To save Optimization parameters cut 1 & 2 minWasteWidth constraint.

unsigned int success = 1;

GlassPlate *plate; // List of solution used plates.
GlassNode *sol_items; // List of solution nodes.
GlassStack *stacks; // List of batch stacks.
GlassItem *items; // List of batch items.


/// Optimization Parameters file Parser
void parseOptimizationParams (string path);


/// Batch file Parser
void parseBatch (string path);


/// Solution file Parser
void parseSolution (string path);


/// Defects file Parser
void parseDefects (string path);


/// Data Structure Builder
void buildDataStructure (GlassNode *node);


/// Data Structure Builder
void checkSuccessors (GlassNode *node, GlassNode *sol_items, int plate_id, GlassNode &parent, int c_end, int index);


/// Create Plate Instance
void createPlate (string *token, int p);


/// Create Node Instance
void createNode (GlassNode * node, string *token, int p);


/// Create Item Instance
void createItem (GlassItem *items, string *token, int p);


/// Display Plates Area Usage
void displayPlatesAreaUsage (void);


/// Verify Identity & Sequence constraints
void verifyIdt_Sequence (void);


/// Verify Dimensions constraint
void verifyDimensions (void);


/// Verify Dimensions constraint of a node successors
void checkSuccDimensions (GlassNode parent);


/// Verify Defects overlap constraint
void verifyDefects (void);


/// Verify Item Production constraint
void verifyItemProduction (void);


/// Create statisticsLog log file
void statisticsLog (void);

/// List violated constraints.
void violatedConstraints (void);

int main (void)
{
    testPath = "";
    string batchPath, solutionPath, defectsPath, optParamsPath;
    int not_exit = 1, choice = 0;
    cout << endl << "\t------------------- Optimization Checker -------------------" << endl << endl;
    log_file << endl << "\t------------------- Optimization Checker -------------------" << endl;

    cout << "\tPlease enter your USED BATCH file Index: ";
    cin >> file_idx;
    batchPath =     PATH_TO_INSTANCES + file_idx + "_batch.csv";
    solutionPath =  PATH_TO_SOLUTIONS + file_idx + "_solution.csv";
    defectsPath =   PATH_TO_INSTANCES + file_idx + "_defects.csv";
    optParamsPath = PATH_TO_INSTANCES "global_param.csv";

    /// If csv files exist and files index is equal.
    string log_name = "logs\\" + file_idx + "_log.txt";
    log_file.open(log_name.c_str());
    cout << endl << "\t\t----------- Start files parsing -----------" << endl;
    log_file << endl << "\t\t----------- Start files parsing -----------" << endl;
    parseOptimizationParams (optParamsPath);
    parseBatch (batchPath);
    parseSolution (solutionPath);
    parseDefects (defectsPath);
    /// If a needed file do not exist.
    if (!opened_file)
        return EXIT_FAILURE;

    cout <<     "\t\t----------- End of files parsing ----------" << endl;
    log_file << "\t\t----------- End of files parsing ----------" << endl;
    // Auto verify constraints and generate log file.
    verifyItemProduction ();
    verifyDefects ();
    verifyIdt_Sequence ();
    verifyDimensions ();
    displayPlatesAreaUsage ();
    violatedConstraints ();
    active_log = 0;
    log_file << endl    << "\t------------------------------------------------------------" << endl;
    if (constraint_error != 0)
        log_file        <<   "                       INVALID SOLUTION                     " << endl;
    else
        log_file        <<  "               SOLUTION VALIDATED SUCCESSFULLY               " << endl;
    log_file            << "\t------------------------------------------------------------" << endl;
    log_file.close ();
    statisticsLog ();
    cout << endl << endl << "\t\t------ Start constraints verification -----" << endl ;
    while (not_exit)
    {
        cout << endl << "\tPress to verify constraint: " << endl;
        cout << "\t1 - Items production" << endl;
        cout << "\t2 - Defects superposing" << endl;
        cout << "\t3 - Identity and Sequence" << endl;
        cout << "\t4 - Items dimensions" << endl;
        cout << "\t5 - All constraints (1-4)" << endl;
        cout << "\t6 - Display plates wasted & used surface (%)" << endl;
        cout << "\t0 - Exit" << endl;
        cout << "\tPressed key: ";
        cin >> choice;
        switch (choice)
        {
        case 1:
            verifyItemProduction ();
            break;
        case 2:
            verifyDefects ();
            break;
        case 3:
            verifyIdt_Sequence ();
            break;
        case 4:
            verifyDimensions ();
            break;
        case 6:
            displayPlatesAreaUsage ();
            break;
        case 5:
            verifyItemProduction ();
            verifyDefects ();
            verifyIdt_Sequence ();
            verifyDimensions ();
            violatedConstraints ();
            cout << endl    << "\t------------------------------------------------------------" << endl;
            if (constraint_error != 0)
                cout        <<   "                       INVALID SOLUTION                     " << endl;
            else
                cout        <<  "               SOLUTION VALIDATED SUCCESSFULLY               " << endl;
            cout            << "\t------------------------------------------------------------" << endl;
            break;
        case 0:
            not_exit = 0;
            break;
        default:
            cout << "\t\tInvalid key" << endl;
            break;
        }
    }
    cout << "\t\t------ End of constraints verification ----" << endl ;
    cout << endl << "\t------------------------------------------------------------" << endl;
    return EXIT_SUCCESS;
}

/* ==================================================================================== */
/*                           StatisticsLog create function                                */
/* ==================================================================================== */
/**
 * This function creates a csv document containing instanceId, valideSolution, nPlates,
 * totalGeoLoss and widthResidual attributes value
**/
void statisticsLog (void)
{
    unsigned int i;
    double waste, total_waste_percentage = 0.0;
    string temp;
    total_waste = 0.0;
    string statisticsPath = "logs\\" + file_idx + "_statistics.csv";
    statistics_file.open (statisticsPath.c_str());
    statistics_file << "instanceId;validSolution;nPlates;totalGeoLoss;widthResidual" << endl;

    statistics_file << file_idx << ";";

    if (constraint_error == 0)
        statistics_file << 1 << ";";
    else
        statistics_file << 0 << ";";

    statistics_file << plates_nbr << ";";
    for (i = 0; i < plates_nbr; i++)
    {
      if (i == plates_nbr-1) // If this is the last plate.
        waste = (double) plate[i].Getwaste()/((plate_w - plate[i].Getresidual().Getwidth())*plate_h/100); // Compute plate[i] wasted area percentage without taking into account residual area.
      else
        waste = (double) plate[i].Getwaste()/(plate_h*plate_w/100); // Compute plate[i] wasted area percentage.
      total_waste += waste;
    }
    total_waste_percentage = (double) total_waste/plates_nbr;
    statistics_file << total_waste_percentage << "%;" << plate[plates_nbr-1].Getresidual().Getwidth() << endl;
    statistics_file.close ();
}


/* ==================================================================================== */
/*                             Display Solution Results                                 */
/* ==================================================================================== */

/**
 * This function display a list of solution's violated constraints.
**/
void violatedConstraints (void)
{
    // If constraints violation occured.
    if (constraint_error != 0)
    {
        cout << endl    << "\t------------------------------------------------------------" << endl;
        cout            << "\t                  Violated Constraints List                 " << endl;
        cout            << "\t------------------------------------------------------------" << endl;

        log_file << endl    << "\t------------------------------------------------------------" << endl;
        log_file            << "\t                  Violated Constraints List                 " << endl;
        log_file            << "\t------------------------------------------------------------" << endl;

        // If Solution tree structure field set to 1
        if ((constraint_error & PARSE_ERROR_MASK) >> PARSE_ERROR_OFFSET)
        {
            cout << "\t- Solution Tree Structure Constraint violated" << endl;
            log_file << "\t- Solution Tree Structure Constraint violated" << endl;
        }

        // If Item production error field set to 1
        if ((constraint_error & ITEM_PRODUCTION_ERROR_MASK) >> ITEM_PRODUCTION_ERROR_OFFSET)
        {
            cout << "\t- Items production Constraint violated" << endl;
            log_file << "\t- Items production Constraint violated" << endl;
        }

        // If Defects superposing error field set to 1
        if ((constraint_error & DEFECTS_SUPERPOSING_ERROR_MASK) >> DEFECTS_SUPERPOSING_ERROR_OFFSET)
        {
            cout << "\t- Defects Superposing Constraint violated" << endl;
            log_file << "\t- Defects Superposing Constraint violated" << endl;
        }

        // If Nodes dimensions error field set to 1
        if ((constraint_error & DIMENSIONS_ERROR_MASK) >> DIMENSIONS_ERROR_OFFSET)
        {
            cout << "\t- Nodes Dimensions Constraint violated" << endl;
            log_file << "\t- Nodes Dimensions Constraint violated" << endl;
        }

        // If Sequence error field set to 1
        if ((constraint_error & SEQUENCE_ERROR_MASK) >> SEQUENCE_ERROR_OFFSET)
        {
            cout << "\t- Sequence Constraint violated" << endl;
            log_file << "\t- Sequence Constraint violated" << endl;
        }

        // If Identity error field set to 1
        if ((constraint_error & IDENTITY_ERROR_MASK) >> IDENTITY_ERROR_OFFSET)
        {
            cout << "\t- Identity Constraint violated" << endl;
            log_file << "\t- Identity Constraint violated" << endl;
        }

        cout            << "\t------------------------------------------------------------" << endl;
        log_file        << "\t------------------------------------------------------------" << endl;
    }
}

/**
 * This function show usage percentage of each plate
 * and a total computation of used/lost area of all used plates
 * Note: this function is accessible if 0 errors are reported by checker
 * verify function (dimensions, identity&sequence, item production and defects superposing)
**/
void displayPlatesAreaUsage (void)
{
    double useful = 0.0;
    double waste = 0.0;
    double total_useful_percentage = 0.0;
    double total_waste_percentage = 0.0;
    unsigned int all_plates_area = 0;
    double residual_percentage = 0.0;
    if (active_log)
        log_file << endl << "\t--- Display Plates Area Usage ---" << endl;
    else
        cout << endl << "\t--- Display Plates Area Usage ---" << endl;
    unsigned int i;
    total_waste = 0, total_useful = 0;
    if (active_log)
    {
        if (constraint_error)
        {
            log_file << "\tERROR -- Unable to verify Display Plates Area Usage due to INVALID SOLUTION" << endl;
            log_file << "\t--- End of Display Plates Area Usage ---" << endl;
            return;
        }
        log_file << "\t---------------------- Summary --------------------------" << endl;
        log_file << "\t|  Plates               |  Used Area    |  Wasted Area  |" << endl;
        log_file << "\t---------------------------------------------------------" << endl;
        for (i = 0; i < plates_nbr; i++)
        {
          if (i == plates_nbr-1) // If this is the last plate.
          {
            useful = (double) plate[i].Getuseful()/((plate_w - plate[i].Getresidual().Getwidth())*plate_h/100); // Compute plate[i] used area percentage without taking into account residual area.
            waste = (double) plate[i].Getwaste()/((plate_w - plate[i].Getresidual().Getwidth())*plate_h/100); // Compute plate[i] wasted area percentage without taking into account residual area.
          }
          else
          {
            useful = (double) plate[i].Getuseful()/(plate_h*plate_w/100); // Compute plate[i] used area percentage.
            waste = (double) plate[i].Getwaste()/(plate_h*plate_w/100); // Compute plate[i] wasted area percentage.
          }
          total_useful += plate[i].Getuseful();
          total_waste += plate[i].Getwaste();
          log_file << "\t|  Plate " << i << "\t\t|  " << useful << "%\t|  " << waste << "%\t|" << endl;
        }
        all_plates_area = (plate_w * plate_h) * (plates_nbr - 1) + (plate_w - plate[plates_nbr-1].Getresidual().Getwidth()) * plate_h;
        total_useful_percentage = (double) 100*total_useful/all_plates_area;
        total_waste_percentage = (double) 100*total_waste/all_plates_area;

        log_file << "\t---------------------------------------------------------" << endl;
        log_file << "\t|  Total (" << plates_nbr << " plate)\t|  " << total_useful_percentage << "%\t|  " << total_waste_percentage << "%\t|" << endl;
        log_file << "\t---------------------------------------------------------" << endl << endl;

        residual_percentage = plate[plates_nbr-1].Getresidual().Getwidth() * plate[plates_nbr-1].Getresidual().Getheight()/(plate_h*plate_w/100);

        log_file << "\t------------------------ Residual -----------------------" << endl;
        log_file << "\t|  Plate id             |  Width        |  Area %       |" << endl;
        log_file << "\t---------------------------------------------------------" << endl;
        log_file << "\t|  Plate " << plate[plates_nbr-1].Getplate_id() << "\t\t|  " << plate[plates_nbr-1].Getresidual().Getwidth() << "mm  \t|  " << residual_percentage << "%  \t|" << endl;
        log_file << "\t---------------------------------------------------------" << endl;
        log_file << "\t--- End of Display Plates Area Usage ---" << endl;
    }
    else
    {
        if (constraint_error)
        {
            cout  << "\tERROR -- Unable to Display Plates Area Usage due to INVALID SOLUTION" << endl;
            cout << "\t--- End of Display Plates Area Usage ---" << endl;
            return;
        }
        cout << "\t---------------------- Summary --------------------------" << endl;
        cout << "\t|  Plates               |  Used Area    |  Wasted Area  |" << endl;
        cout << "\t---------------------------------------------------------" << endl;
        for (i = 0; i < plates_nbr; i++)
        {
            if (i == plates_nbr-1) // If this is the last plate.
              {
                useful = (double) plate[i].Getuseful()/((plate_w - plate[i].Getresidual().Getwidth())*plate_h/100); // Compute plate[i] used area percentage without taking into account residual area.
                waste = (double) plate[i].Getwaste()/((plate_w - plate[i].Getresidual().Getwidth())*plate_h/100); // Compute plate[i] wasted area percentage without taking into account residual area.
              }
              else
              {
                useful = (double) plate[i].Getuseful()/(plate_h*plate_w/100); // Compute plate[i] used area percentage.
                waste = (double) plate[i].Getwaste()/(plate_h*plate_w/100); // Compute plate[i] wasted area percentage.
              }
              total_useful += plate[i].Getuseful();
              total_waste += plate[i].Getwaste();
              cout << "\t|  Plate " << i << "\t\t|  " << useful << "%\t|  " << waste << "%\t|" << endl;
        }
        all_plates_area = (plate_w * plate_h) * (plates_nbr - 1) + (plate_w - plate[plates_nbr-1].Getresidual().Getwidth()) * plate_h;
        total_useful_percentage = (double) 100*total_useful/all_plates_area;
        total_waste_percentage = (double) 100*total_waste/all_plates_area;

        cout << "\t---------------------------------------------------------" << endl;
        cout << "\t|  Total (" << plates_nbr << " plate)\t|  " << total_useful_percentage  << "%\t|  " << total_waste_percentage  << "%\t|" << endl;
        cout << "\t---------------------------------------------------------" << endl << endl;

        residual_percentage = plate[plates_nbr-1].Getresidual().Getwidth() * plate[plates_nbr-1].Getresidual().Getheight()/(plate_h*plate_w/100);

        cout << "\t------------------------ Residual -----------------------" << endl;
        cout << "\t|  Plate id             |  Width        |  Area %       |" << endl;
        cout << "\t---------------------------------------------------------" << endl;
        cout << "\t|  Plate " << plate[plates_nbr-1].Getplate_id() << "\t\t|  " << plate[plates_nbr-1].Getresidual().Getwidth() << "mm  \t|  " << residual_percentage << "%  \t|" << endl;
        cout << "\t---------------------------------------------------------" << endl;
        cout << "\t--- End of Display Plates Area Usage ---" << endl;
    }
}



/* ==================================================================================== */
/*                       Constraint verification functions                              */
/* ==================================================================================== */
/**
 * This function verifies if all batch items are produced,
 * not produced and/or duplicated.
**/
void verifyItemProduction (void)
{
    if (active_log)
        log_file << "\n\t--- Item production constraint verification ---" << endl;
    else
        cout << endl << "\t--- Item production constraint verification ---" << endl;
    unsigned int i, j, nbr = 0;
    // Loop on batch parsed items.
    for (i = 0; i < batch_items; i++)
    {
        nbr = 0;
        // For each batch item loop on solution parsed nodes.
        for (j = 0; j < useful_node; j++)
        {
            // Test if batch item id is identical to solution node type.
            if (items[i].Getitem_id() == sol_items[j].Gettype())
                nbr++;
        }
        // If batch item is found more than one time, the item is declared duplicated.
        if (nbr > 1)
        {
            if (active_log)
            {
                log_file << "\tERROR -- Item " << items[i].Getitem_id() << ": is duplicated" << endl;
                constraint_error |= ITEM_PRODUCTION_ERROR_MASK;
            }
            else
            {
                cout << "\tERROR -- Item " << items[i].Getitem_id() << ": is duplicated" << endl;
            }
            //break;
        }
        // If batch item is not found at all, the item is declared as not produced.
        if (nbr == 0)
        {
            if (active_log)
            {
                log_file << "\tERROR -- Item " << items[i].Getitem_id() << ": is not produced" << endl;
                constraint_error |= ITEM_PRODUCTION_ERROR_MASK;
            }
            else
            {
                cout << "\tERROR -- Item " << items[i].Getitem_id() << ": is not produced" << endl;
            }
            //break;
        }
    }
    if (active_log)
    {
        if (nbr == 1)
            log_file << "\tItem production constraint verified successfully" << endl;
        else
            log_file << "\tERROR -- Item production constraint violated" << endl;
    }
    else
    {
        if (nbr == 1)
            cout << "\tItem production constraint verified successfully" << endl;
        else
            cout << "\tERROR -- Item production constraint violated" << endl;
    }
    if (active_log)
        log_file <<     "\t--- End of Item production constraint verification ---" << endl;
    else
        cout <<         "\t--- End of Item production constraint verification ---" << endl;
}


/**
 * This function verifies if solution items overlap defect(s)
 * given in defects csv file.
**/
void verifyDefects (void)
{
    unsigned int error = 0;
    if (active_log)
        log_file << endl << "\t--- Superposing with defects constraint verification ---" << endl;
    else
        cout << endl << "\t--- Superposing with defects constraint verification ---" << endl;
    unsigned int i, j, plate_id;
    unsigned int defect_width, defect_height, defect_x, defect_y; // Defect Coord.
    unsigned int item_width, item_height, item_x, item_y; // Item Coord.
    // Loop on solution parsed nodes.
    for (i = 0; i < useful_node; i++)
    {
        // Get node coordinates.
        item_x = sol_items [i].Getpos_x();
        item_y = sol_items [i].Getpos_y();
        item_width = sol_items [i].Getwidth();
        item_height = sol_items [i].Getheight();
        plate_id = sol_items[i].Getplate_id();
        // Loop on associated plate defects.
        for (j = 0; j < plate[plate_id].Getdefect_nbr(); j++)
        {
            // Get defect coordinates.
            defect_x = plate[sol_items[i].Getplate_id()].Getdefect(j).Getpos_x();
            defect_y = plate[sol_items[i].Getplate_id()].Getdefect(j).Getpos_y();
            defect_width = plate[sol_items[i].Getplate_id()].Getdefect(j).Getwidth();
            defect_height = plate[sol_items[i].Getplate_id()].Getdefect(j).Getheight();
            // Test if one or more defect ends are inside the solution useful node area.
            if (  (((item_x <= defect_x) && (defect_x < item_x + item_width)) || ((item_x < defect_x + defect_width) && (defect_x + defect_width < item_x + item_width))) && // horizontal ends
                  (((item_y <= defect_y) && (defect_y < item_y + item_height)) || ((item_y < defect_y + defect_height) && (defect_y + defect_height < item_y + item_height)))  ) // vertical ends
            {
                error = 1;
                if (active_log)
                {
                    log_file << "\tERROR -- Node " << sol_items [i].Getnode_id() << ": (type: item, item_id: " << sol_items [i].Gettype() << ", x: " << item_x << ", y: " << item_y << ", width: " << item_width << ", height: " << item_height
                         << ") superposes defect " << plate[sol_items[i].Getplate_id()].Getdefect(j).Getdefect_id() << ": (x: " << defect_x << ", y: " << defect_y << ", width: " << defect_width << ", height: " << defect_height << ")" << endl;
                    constraint_error |= DEFECTS_SUPERPOSING_ERROR_MASK;
                }
                else
                {
                    cout << "\tERROR -- Node " << sol_items [i].Getnode_id() << ": (type: item, item_id: " << sol_items [i].Gettype() << ", x: " << item_x << ", y: " << item_y << ", width: " << item_width << ", height: " << item_height
                         << ") superposes defect " << plate[sol_items[i].Getplate_id()].Getdefect(j).Getdefect_id() << ": (x: " << defect_x << ", y: " << defect_y << ", width: " << defect_width << ", height: " << defect_height << ")" << endl;
                }
            }
        }
    }
    if (active_log)
    {
        if (error == 0)
            log_file << "\tSuperposing with defects constraint verified successfully" << endl;
        log_file << "\t--- End of Superposing with defects constraint verification ---" << endl;
    }
    else
    {
        if (error == 0)
            cout << "\tSuperposing with defects constraint verified successfully" << endl;
        cout     << "\t--- End of Superposing with defects constraint verification ---" << endl;
    }
}


/**
 * This function verifies dimensions constraint for first node cut level.
 * It verifies nodes superposing, cut stage, if nodes dimensions respect prefixed optimization parameters
 * if node's x, y, width and height are coherent with parent node (dependent on cut stage) and do not overflow it's area,
 * and verifies that nodes widths sum is equal to parent area.
**/
void verifyDimensions (void)
{
    if (active_log)
        log_file << endl << "\t--- Dimensions constraint verification ---" << endl;
    else
        cout << endl << "\t--- Dimensions constraint verification ---" << endl;
    unsigned int i, j, height, width, x, y, cut, c_nbr;
    int type;
    //
    success = 1;
    // Loop on plates list.
    for (i = 0; i < plates_nbr; i++)
    {
        // Get plate successors number.
        c_nbr = plate[i].Getsuccessor_nbr();
        for (j = 0; j < c_nbr; j++)
        {
            // Get successor coordinates and type.
            height = plate[i].Getsuccessor(j).Getheight();
            width = plate[i].Getsuccessor(j).Getwidth();
            x = plate[i].Getsuccessor(j).Getpos_x();
            y = plate[i].Getsuccessor(j).Getpos_y();
            type = plate[i].Getsuccessor(j).Gettype();
            cut = plate[i].Getsuccessor(j).Getcut();
            // If node cut stage do not equal parent.cut + 1.
            if (cut != plate[i].Getcut()+1)
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (type: branch, width: " << width << ", height: " << height << ") has node.cut != plate.cut + 1 (plate.node_id: " << plate[i].Getnode_id() << ")" << endl;
                else
                    cout << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (type: branch, width: " << width << ", height: " << height << ") has node.cut != plate.cut + 1 (plate.node_id: " << plate[i].Getnode_id() << ")" << endl;
                success = 0;
            }
            // If branch node type.
            if (type == -2)
            {
                // verifies width is not < min1Cut optimization parameter.
                if (width < min1Cut)
                {
                    if (active_log)
                        log_file << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (type: branch, width: " << width << ", height: " << height << ") has node.width less than " << min1Cut << endl;
                    else
                        cout << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (type: branch, width: " << width << ", height: " << height << ") has node.width less than " << min1Cut << endl;
                    success = 0;
                }
                // verifies width is not > max1Cut optimization parameter.
                if (width > max1Cut)
                {
                    if (active_log)
                        log_file << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (type: branch, width: " << width << ", height: " << height << ") has node.width greater than " << max1Cut << "mm" << endl;
                    else
                        cout << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (type: branch, width: " << width << ", height: " << height << ") has node.width greater than " << max1Cut << "mm" << endl;
                    success = 0;
                }
            }
            // verifies node.y parameter is equal or not to plate's y position.
            if (y != plate[i].Getpos_y())
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": node.y is not equal to its plate " << plate[i].Getnode_id() << " y: " << plate[i].Getpos_y() << endl;
                else
                    cout << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": node.y is not equal to its plate " << plate[i].Getnode_id() << " y: " << plate[i].Getpos_y() << endl;
                success = 0;
            }
            // verifies node.height parameter is equal or not to plate's height.
            if (height != plate[i].Getheight())
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": node.height is not equal to its plate " << plate[i].Getnode_id() << " height: " << plate[i].Getheight() << endl;
                else
                    cout << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": node.height is not equal to its plate " << plate[i].Getnode_id() << " height: " << plate[i].Getheight() << endl;
                success = 0;
            }
            // verifies if first node of plate[i] starts at coordinate node.x = 0.
            if ((j == 0) && (x != 0))
            {
                if (active_log)
                    cout << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.x != plate.x (plate.node_id: " << plate[i].Getnode_id() << ", x: 0)" << endl;
                else
                    log_file << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.x != plate.x (plate.node_id: " << plate[i].Getnode_id() << ", x: 0)" << endl;
                success = 0;
            }
            // verifies if nodes from the same cut stage have node[i].x == node[i-1].x + node[i-1].width.
            if ( (j > 0) && (x != (plate[i].Getsuccessor(j-1).Getpos_x() + plate[i].Getsuccessor(j-1).Getwidth())))
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.x != node[" << plate[i].Getsuccessor(j-1).Getnode_id() << "].x + node[" << plate[i].Getsuccessor(j-1).Getnode_id() << "].width" << endl;
                else
                    cout << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.x != node[" << plate[i].Getsuccessor(j-1).Getnode_id() << "].x + node[" << plate[i].Getsuccessor(j-1).Getnode_id() << "].width" << endl;
                success = 0;
            }
            // verifies if node.width + node.x overflow plate's width.
            if (x+width > plate[i].Getwidth())
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.x + node.width > plate.width (plate.node_id: " << plate[i].Getnode_id() << ")" << endl;
                else
                    cout << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.x + node.width > plate.width (plate.node_id: " << plate[i].Getnode_id() << ")" << endl;
                success = 0;
            }
            // if node is last plate's successor, verify if node.width + node.x is less than plate's width.
            if ((j == c_nbr-1) && (x+width < plate[i].Getwidth()))
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": is the last of plate " << plate[i].Getplate_id() << " successors and node.x + node.width < plate.width (plate.node_id: " << plate[i].Getnode_id() << ")" << endl;
                else
                    cout << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": is the last of plate " << plate[i].Getplate_id() << " successors and node.x + node.width < plate.width (plate.node_id: " << plate[i].Getnode_id() << ")" << endl;
                success = 0;
            }
            // If node has waste type.
            if (type == -1)
            {
                // verifies if node.height is not inferior to waste_min optimization parameter prefixed value.
                if (height < waste_min)
                {
                    if (active_log)
                        log_file << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (type: waste, width: " << width << ", height: " << height << ") has node.height less than " << waste_min << "mm" << endl;
                    else
                        cout << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (type: waste, width: " << width << ", height: " << height << ") has node.height less than " << waste_min << "mm" << endl;
                    success = 0;
                }
                // verifies if node.width is not inferior to waste_min optimization parameter prefixed value.
                if (width < waste_min)
                {
                    if (active_log)
                        log_file << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (type: waste, width: " << width << ", height: " << height << ") has node.width less than " << waste_min << "mm" << endl;
                    else
                        cout << "\tERROR -- Node " << plate[i].Getsuccessor(j).Getnode_id() << ": (type: waste, width: " << width << ", height: " << height << ") has node.width less than " << waste_min << "mm" << endl;
                    success = 0;
                }
            }
            // If node has residual type, verifies if node.width and node.height are not inferior to waste_min optimization parameter prefixed value.
            if ((i == plates_nbr-1) && (residual_node > 0))
            {
                if (plate[i].Getresidual().Getwidth() < waste_min)
                {
                    if (active_log)
                        log_file << "\tERROR -- Node " << plate[i].Getresidual().Getnode_id() << ": (type: residual, width: " << plate[i].Getresidual().Getwidth() << ", height: " << plate[i].Getresidual().Getheight() << ") has node.width less than " << waste_min << "mm" << endl;
                    else
                        cout << "\tERROR -- Node " << plate[i].Getresidual().Getnode_id() << ": (type: residual, width: " << plate[i].Getresidual().Getwidth() << ", height: " << plate[i].Getresidual().Getheight() << ") has node.width less than " << waste_min << "mm" << endl;
                    success = 0;
                }
            }
            // If node has branch type, look for its successors dimensions.
            if (type == -2)
                checkSuccDimensions (plate[i].Getsuccessor(j));
        }
    }
    if (success)
    {
        if (active_log)
        {
            log_file << "\tDimension constraint verified successfully"<< endl;
        }
        else
        {
            cout << "\tDimension constraint verified successfully"<< endl;
        }
    }
    else
        constraint_error |= DIMENSIONS_ERROR_MASK;
    if (active_log)
        log_file <<         "\t--- End of Dimension constraint verification ---" << endl;
    else
        cout <<         "\t--- End of Dimension constraint verification ---" << endl;
}


/**
 * This function is recursive, it verifies dimensions constraint of node successors.
**/
void checkSuccDimensions (GlassNode parent)
{
    unsigned int j, height, width, x, y, cut, c_nbr;
    int type;
    // Get node successors number (children).
    c_nbr = parent.Getsuccessor_nbr();
    // Loop on all node successors
    for (j = 0; j < c_nbr; j++)
    {
        // Get successor coordinates.
        height = parent.Getsuccessor(j).Getheight();
        width = parent.Getsuccessor(j).Getwidth();
        x = parent.Getsuccessor(j).Getpos_x();
        y = parent.Getsuccessor(j).Getpos_y();
        cut = parent.Getsuccessor(j).Getcut();
        type = parent.Getsuccessor(j).Gettype();
        // If node cut is not equal to parent.cut + 1.
        if (cut != parent.Getcut()+1)
        {
            if (active_log)
                log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (type: branch, width: " << width << ", height: " << height << ") has node.cut != parent.cut + 1 (parent.node_id: " << parent.Getnode_id() << ")" << endl;
            else
                cout <<     "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (type: branch, width: " << width << ", height: " << height << ") has node.cut != parent.cut + 1 (parent.node_id: " << parent.Getnode_id() << ")" << endl;
            success = 0;
        }
        // If node has waste type and cut stage equals 2.
        if (type == -1)
        {
            // verifies if node.height is not inferior to waste_min optimization parameter prefixed value.
            if (height < waste_min)
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (type: waste, width: " << width << ", height: " << height << ") has node.height less than " << waste_min << "mm" << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (type: waste, width: " << width << ", height: " << height << ") has node.height less than " << waste_min << "mm" << endl;
                success = 0;
            }
            // verifies if node.width is not inferior to waste_min optimization parameter prefixed value.
            if (width < waste_min)
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (type: waste, width: " << width << ", height: " << height << ") has node.width less than " << waste_min << "mm" << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (type: waste, width: " << width << ", height: " << height << ") has node.width less than " << waste_min << "mm" << endl;
                success = 0;
            }
        }
        // verifies if node cut stage equals 2 and node.height is inferior to min2Cut optimization parameter prefixed value.
        if ((cut == 2) && (type == -2) && (height < min2Cut))
        {
            if (active_log)
                log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (type: branch, width: " << width << ", height: " << height << ") has node.height less than " << min2Cut << "mm" << endl;
            else
                cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (type: branch, width: " << width << ", height: " << height << ") has node.height less than " << min2Cut << "mm" << endl;
            success = 0;
        }
        // if odd cut stage.
        if (cut%2)
        {
            // verifies if node.y is different than it's parent.pos_y value.
            if (y != parent.Getpos_y())
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": node.y is not equal to its parent node " << parent.Getnode_id() << " y: " << parent.Getpos_y() << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": node.y is not equal to its parent node " << parent.Getnode_id() << " y: " << parent.Getpos_y() << endl;
                success = 0;
            }
            // verifies if node.height is different than it's parent.height value.
            if (height != parent.Getheight())
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": node.height is not equal to its parent node " << parent.Getnode_id() << " height: " << parent.Getheight() << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": node.height is not equal to its parent node " << parent.Getnode_id() << " height: " << parent.Getheight() << endl;
                success = 0;
            }
            // verifies if node[j] is the first successor and node[j].x is different than parent.x.
            if ((j == 0) && (x != parent.Getpos_x()))
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.x != parent.x / parent is Node "<< parent.Getnode_id() << "( x: " << parent.Getpos_x() << ", y: " << parent.Getpos_y() << " width: " << parent.Getwidth() << ", height: "<< parent.Getheight() << ")" << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.x != parent.x / parent is Node "<< parent.Getnode_id() << "( x: " << parent.Getpos_x() << ", y: " << parent.Getpos_y() << " width: " << parent.Getwidth() << ", height: "<< parent.Getheight() << ")" << endl;
                success = 0;
            }
            // verifies if node[j] is not the first successor and node[j].x is different than node[j-1].x + node[j-1].width.
            if ( (j > 0) && (x != (parent.Getsuccessor(j-1).Getpos_x() + parent.Getsuccessor(j-1).Getwidth())))
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.x != node[" << parent.Getsuccessor(j-1).Getnode_id() << "].x + node[" << parent.Getsuccessor(j-1).Getnode_id() << "].width" << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.x != node[" << parent.Getsuccessor(j-1).Getnode_id() << "].x + node[" << parent.Getsuccessor(j-1).Getnode_id() << "].width" << endl;
                success = 0;
            }
            // verifies if node.x + node.width do not overlap parent node area.
            if (x+width > parent.Getwidth()+parent.Getpos_x())
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": node.x + node.width > parent.x + parent.width (parent.node_id: " << parent.Getnode_id() << ")" << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": node.x + node.width > parent.x + parent.width (parent.node_id: " << parent.Getnode_id() << ")" << endl;
                success = 0;
            }
            // verifies if node[j] is not the last successor and node[j].x + node[j].width less than parent.x + parent.width.
            if ((j == c_nbr-1) && (x+width < parent.Getwidth()+parent.Getpos_x()))
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": is the last of node " << parent.Getnode_id() << " successors and node.x + node.width < parent.x + parent.width (parent.node_id: " << parent.Getnode_id() << ")" << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": is the last of node " << parent.Getnode_id() << " successors and node.x + node.width < parent.x + parent.width (parent.node_id: " << parent.Getnode_id() << ")" << endl;
                success = 0;
            }
        }
        else // if even cut stage.
        {
            // if node.x is different from parent.x.
            if (x != parent.Getpos_x())
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": x: " << parent.Getsuccessor(j).Getpos_x() << " is not equal to its parent node " << parent.Getnode_id() << " x: " << parent.Getpos_x() << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": x: " << parent.Getsuccessor(j).Getpos_x() << " is not equal to its parent node " << parent.Getnode_id() << " x: " << parent.Getpos_x() << endl;
                success = 0;
            }
            // if node.width is different from parent.width.
            if (width != parent.Getwidth())
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": node.width is not equal to its parent node " << parent.Getnode_id() << " width: " << parent.Getwidth() << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": node.width is not equal to its parent node " << parent.Getnode_id() << " width: " << parent.Getwidth() << endl;
                success = 0;
            }
            // if node[j] is first parent successor and node.y is different from parent.y.
            if ((j == 0) && (y != parent.Getpos_y()))
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.y != parent.y / parent is Node "<< parent.Getnode_id() << "( x: " << parent.Getpos_x() << ", y: " << parent.Getpos_y() << " width: " << parent.Getwidth() << ", height: "<< parent.Getheight() << ")" << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.y != parent.y / parent is Node "<< parent.Getnode_id() << "( x: " << parent.Getpos_x() << ", y: " << parent.Getpos_y() << " width: " << parent.Getwidth() << ", height: "<< parent.Getheight() << ")" << endl;
                success = 0;
            }
            // if node[j] is not the first parent successor and node.y is different from node[j-1].y + node[j-1].height.
            if ((j > 0) && (y != (parent.Getsuccessor(j-1).Getpos_y() + parent.Getsuccessor(j-1).Getheight())))
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.y != node[" << parent.Getsuccessor(j-1).Getnode_id() << "].y + node[" << parent.Getsuccessor(j-1).Getnode_id() << "].height" << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": (x: " << x << ", y: " << y << ", width: " << width << ", height: " << height << ") has node.y != node[" << parent.Getsuccessor(j-1).Getnode_id() << "].y + node[" << parent.Getsuccessor(j-1).Getnode_id() << "].height" << endl;
                success = 0;
            }
            // if node.y + node.height overlap parent area.
            if (height+y > parent.Getheight()+parent.Getpos_y())
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": node.y + node.height > parent.y + parent.height (parent.node_id: " << parent.Getnode_id() << ")" << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": node.y + node.height > parent.y + parent.height (parent.node_id: " << parent.Getnode_id() << ")" << endl;
                success = 0;
            }
            // if node[j] is last successor ans node.y + node.height is inferior to parent.y + parent.height .
            if ((j == c_nbr-1) && (y+height < parent.Getheight() + parent.Getpos_y()))
            {
                if (active_log)
                    log_file << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": is the last of node " << parent.Getnode_id() << " successors and node.y + node.height < parent.y + parent.height (parent.node_id: " << parent.Getnode_id() << ")" << endl;
                else
                    cout << "\tERROR -- Node " << parent.Getsuccessor(j).Getnode_id() << ": is the last of node " << parent.Getnode_id() << " successors and node.y + node.height < parent.y + parent.height (parent.node_id: " << parent.Getnode_id() << ")" << endl;
                success = 0;
            }
        }
        if (type == -2)
            checkSuccDimensions (parent.Getsuccessor(j));
    }
    if (!success)
    {
        constraint_error |= DIMENSIONS_ERROR_MASK;
    }
}


/**
 * This function verifies the Identity and sequence constraints
 * Identity:
 *      this function verifies that all solution items are identical to batch items
 *      with the possibility to invert width and height.
 * Sequence:
 *      this function verifies that user solution respects the production sequence
 *      given in the batch file.
**/
void verifyIdt_Sequence(void)
{
    if (active_log)
        log_file << endl << "\t--- Sequence & Identity constraints verification ---" << endl;
    else
        cout << endl << "\t--- Sequence & Identity constraints verification ---" << endl;
    unsigned int i, j, sol_width, sol_height, batch_width, batch_height, fnd_stck = 0, idtty_error = 0;
    unsigned int stack_cur_idx;
    // Verify that no Item production errors occured.
    if ((constraint_error & ITEM_PRODUCTION_ERROR_MASK) >> ITEM_PRODUCTION_ERROR_OFFSET)
    {
        if (active_log)
        {
            log_file << "\tERROR -- Unable to verify Sequence & Identity constraints due to Item Production ERROR" << endl;
        }
        else
            cout << "\tERROR -- Unable to verify Sequence & Identity constraints due to Item Production ERROR" << endl;
    }
    else
    {
        // Loop on all solution nodes.
        for (i = 0; i < useful_node; i++)
        {
            fnd_stck = 0, idtty_error = 0;
            // Loop on all batch stacks.
            for (j = 0; j < stack_nbr; j++)
            {
                stack_cur_idx = stacks[j].Getcur_item_idx();
                // If Getcur_item_idx() call returned 0xffff it means that all items of stack[i] are already verified and no items left.
                if (stack_cur_idx == 0xffff)
                    break;
                // If solution item type equals to a authorized stack item id.
                if (sol_items[i].Gettype() == stacks[j].Getitem(stack_cur_idx).Getitem_id())
                {
                    fnd_stck++;
                    sol_width = sol_items[i].Getwidth();
                    sol_height = sol_items[i].Getheight();
                    batch_width = stacks[j].Getitem(stacks[j].Getcur_item_idx()).Getitem_w ();
                    batch_height = stacks[j].Getitem(stacks[j].Getcur_item_idx()).Getitem_h ();
                    stacks[j].Increasecur_item_idx();
                    if (!(((sol_height == batch_height) && (sol_width == batch_width)) || ((sol_height == batch_width) && (sol_width == batch_height))))
                    {
                        if (active_log)
                            log_file << "\tERROR -- Identity constraint is not respected " << endl << "\t\tItem " << sol_items[i].Gettype() << " ( width: " << sol_width << ", height: " << sol_height << ")  parameters are not identical to its batch version (width: " << batch_width << ", height: " << batch_height << ")." << endl;
                        else
                            cout << "\tERROR -- Identity constraint is not respected " << endl << "\t\tItem " << sol_items[i].Gettype() << " ( width: " << sol_width << ", height: " << sol_height << ")  parameters are not identical to its batch version (width: " << batch_width << ", height: " << batch_height << ")." << endl;
                        idtty_error ++;
                        constraint_error |= IDENTITY_ERROR_MASK;
                    }
                }
            }
            // If solution item is not a authorized item (not found).
            if ( fnd_stck == 0)
            {
                if (active_log)
                {
                    log_file << "\tERROR -- Sequence constraint is not respected " << endl << "\t\tItem " << sol_items[i].Gettype() << " is not allowed" << endl;
                    constraint_error |= SEQUENCE_ERROR_MASK;
                }
                else
                    cout << "\tERROR -- Sequence constraint is not respected " << endl << "\t\tItem " << sol_items[i].Gettype() << " is not allowed" << endl;
                break;
            }
        }
        // Clear stacks current item index to allow a second run of the test.
        for (j = 0; j < stack_nbr; j++)
        {
            stacks[j].Setcur_item_idx(0);
        }
        // if batch items sequence is respected.
        if (fnd_stck == 1)
        {
            if (active_log)
                log_file << "\tSequence Constraint verified successfully" << endl;
            else
                cout << "\tSequence Constraint verified successfully" << endl;
        }
        // if solution useful nodes are identical to batch items.
        if (idtty_error == 0)
        {
            if (active_log)
                log_file << "\tIdentity Constraint verified successfully" << endl;
            else
                cout << "\tIdentity Constraint verified successfully" << endl;
        }
    }
    if (active_log)
        log_file << "\t--- End of Sequence & Identity constraints verification ---" << endl;
    else
        cout << "\t--- End of Sequence & Identity constraints verification ---" << endl;
}


/* ==================================================================================== */
/*                             CSV Parse files functions                                */
/* ==================================================================================== */
/**
 * This function parse optimization parameters csv file
 * to obtain prefixed value of those parameter.
 * path: input / path to optimization parameters file.
**/
void parseOptimizationParams(string path)
{
    unsigned int i = 0;
    string value, temp, token [OPT_PARAM_RAWS_LENGTH];
    ifstream filess (path.c_str());
    if (!filess.is_open())
    {
        opened_file = false;
        cout << endl << "\t--- Unable to Open Optimization Parameters file / file not found ---" << endl << "\tOptimization Parameters file path: " << path << endl;
        log_file << endl << "\t--- Unable to Open Optimization Parameters file / file not found ---" << endl << "\tOptimization Parameters file path: " << path << endl;
        cout <<     "\t--- Optimization Parameters file not parsed ---" << endl;
        log_file << "\t--- Optimization Parameters file not parsed ---" << endl;
        return;
    }
    cout << endl << "\t--- Parsing Optimization Parameters file ---" << endl << "\tOptimization Parameters file path: " << path << endl;
    log_file << endl << "\t--- Parsing Optimization Parameters file ---" << endl << "\tOptimization Parameters file path: " << path << endl;
    getline (filess, value, '\n'); // skip first line.
    // Get nPlates parameter.
    while (filess.good())
    {
        getline (filess, value, '\n');
        temp = value.substr(0, value.find(';'));
        if (temp.size())
            token[i++] = value.erase(0, temp.size()+1);
    }
    plate_nbr_limit = atoi(token[OPT_PARAM_NPLATES_RAW].c_str());
    plate_w = atoi(token[OPT_PARAM_WIDTHPLATES_RAW].c_str());
    plate_h = atoi(token[OPT_PARAM_HEIGHTPLATES_RAW].c_str());
    min1Cut = atoi(token[OPT_PARAM_MIN1CUT_RAW].c_str());
    min2Cut = atoi(token[OPT_PARAM_MIN2CUT_RAW].c_str());
    max1Cut = atoi(token[OPT_PARAM_MAX1CUT_RAW].c_str());
    waste_min = atoi(token[OPT_PARAM_MINWASTE_RAW].c_str());
    cout << "\tnPlates: " << plate_nbr_limit << endl
         << "\twidthPlates: " << plate_w << endl
         << "\theightPlates: " << plate_h << endl
         << "\tmin1Cut: " << min1Cut << endl
         << "\tmax1Cut: " << max1Cut << endl
         << "\tmin2Cut: " << min2Cut << endl
         << "\tminWaste: " << waste_min << endl;
    log_file << "\tnPlates: " << plate_nbr_limit << endl
         << "\twidthPlates: " << plate_w << endl
         << "\theightPlates: " << plate_h << endl
         << "\tmin1Cut: " << min1Cut << endl
         << "\tmax1Cut: " << max1Cut << endl
         << "\tmin2Cut: " << min2Cut << endl
         << "\tminWaste: " << waste_min << endl;
    cout << "\t--- Optimization Parameters file parsed successfully ---" << endl;
    log_file << "\t--- Optimization Parameters file parsed successfully ---" << endl;
    filess.close();
}


/**
 * This function parse Defects csv file.
 * path: input / path to Defects file.
**/
void parseDefects(string path)
{
    GlassDefect defect;
    unsigned int plate_defect [PLATES_NBR_LIMIT][2];
    unsigned int i, k, s = 0, defects_on_used_plats = 0;
    string token[DEFECTS_COL_LENGTH], temp;
    string value;
    ifstream filess (path.c_str());
    if (!filess.is_open())
    {
        opened_file = false;
        cout << endl << "\t--- Unable to Open Defects file / file not found ---" << endl << "\tDefects file path: " << path << endl;
        log_file << endl << "\t--- Unable to Open Defects file / file not found ---" << endl << "\tDefects file path: " << path << endl;
        cout <<     "\t--- Defects file not parsed ---" << endl;
        log_file << "\t--- Defects file not parsed ---" << endl;
        return;
    }
    cout << endl << "\t--- Parsing Defects file ---" << endl << "\tDefects file path: " << path << endl;
    log_file << endl << "\t--- Parsing Defects file ---" << endl << "\tDefects file path: " << path << endl;
    // Skip first line.
    for (i = 0; i < plates_nbr; i++)
        plate_defect[i][1] = 0;
    getline (filess, value, '\n');
    // Parse defects file.
    while (filess.good())
    {
        getline (filess, value, '\n');
        temp = value.substr(0, value.find(';'));
        if (temp.size())
        {
            defects_nbr++;
            temp = value.erase(0, temp.size()+1);
            temp = temp.substr(0, value.find(';'));
            if (atoi(temp.c_str()) < plates_nbr)
                plate_defect[atoi(temp.c_str())][1]++;
        }
    }
    // Allocate defects array to each used plate.
    for (i = 0; i < plates_nbr; i++)
    {
        plate[i].defect = new GlassDefect[plate_defect[i][1]];
        // Compute defects on used plates.
        defects_on_used_plats += plate_defect[i][1];
    }
    cout <<     "\tDefects: " << defects_nbr << endl << "\tDefects on used plates: " << defects_on_used_plats << endl;
    log_file << "\tDefects: " << defects_nbr << endl << "\tDefects on used plates: " << defects_on_used_plats << endl;
    filess.clear ();
    filess.seekg (0, filess.beg);
    getline (filess, value, '\n');
    while (filess.good())
    {
        getline (filess, value, '\n');
        // Loop on value string to split defects column tokens.
        for (i = 0; i < value.size();)
        {
            for (k = i; k < value.size(); k++)
            {
                if(value[k] == ';')
                {
                    i = k + 1;
                    s = (s+1)%6;
                    break;
                }
                else if (k == (value.size()-1) && (s == 5))
                {
                    token[s].append (value, k, 1);
                    i = k+1;
                    break;
                }
                else
                {
                    token[s].append (value, k, 1);
                }
            }
        }
        if (token[DEFECTS_DEFECT_ID_COL].size() && (atoi(token[DEFECTS_PLATE_ID_COL].c_str()) < plates_nbr))
        {
            // Create defect with parsed parameters.
            GlassDefect d (atoi(token[DEFECTS_DEFECT_ID_COL].c_str()),
                           atoi(token[DEFECTS_PLATE_ID_COL].c_str()),
                           atoi(token[DEFECTS_X_COL].c_str()),
                           atoi(token[DEFECTS_Y_COL].c_str()),
                           atoi(token[DEFECTS_WIDTH_COL].c_str()),
                           atoi(token[DEFECTS_HEIGHT_COL].c_str()));
            // Add defect to respective plate.
            plate[atoi(token[DEFECTS_PLATE_ID_COL].c_str())].Setdefect(d);
        }
        else
        {
            break;
        }
        for (k = 0; k < DEFECTS_COL_LENGTH; k++)
        {
            token[k].clear ();
        }
        s = 0;
    }
    filess.close();
    cout << "\t--- Defects file parsed successfully ---" << endl << endl;
    log_file << "\t--- Defects file parsed successfully ---" << endl << endl;
}


/**
 * This function parse batch csv file to obtain a
 * prefixed production sequence of items and stacks.
 * path: input / path to batch file.
**/
void parseBatch (string path)
{
    unsigned int j, i, k, p = 0, s = 0;
    int n = -1, first = 0, init = 0;
    string token[BATCH_COL_LENGTH], temp;
    string value;
    ifstream filess (path.c_str());
    if (!filess.is_open())
    {
        opened_file = false;
        cout << endl << "\t--- Unable to Open Batch file / file not found ---" << endl << "\tBatch file path: " << path << endl;
        log_file << endl << "\t--- Unable to Open Batch file / file not found ---" << endl << "\tBatch file path: " << path << endl;
        cout <<     "\t--- Batch file not parsed ---" << endl;
        log_file << "\t--- Batch file not parsed ---" << endl;
        return;
    }
    cout << endl << "\t--- Parsing Batch file ---" << endl << "\tBatch file path: " << path << endl;
    log_file << endl << "\t--- Parsing Batch file ---" << endl << "\tBatch file path: " << path << endl;
    // Skip first line.
    getline (filess, value, '\n');
    while (filess.good())
    {
        getline (filess, value, '\n');
        temp = value.substr(0, value.find(';'));
        if (temp.size())
            batch_items++;
    }
    items = new GlassItem[batch_items];
    cout << "\tItems: " << batch_items << endl;
    log_file << "\tItems: " << batch_items << endl;
    filess.clear ();
    filess.seekg (0, filess.beg);
    getline (filess, value, '\n');
    while (filess.good())
    {
        getline (filess, value, '\n');
        // Loop on value string to split batch column tokens.
        for (i = 0; i < value.size();)
        {
            for (k = i; k < value.size(); k++)
            {
                if(value[k] == ';')
                {
                    i = k + 1;
                    s = (s+1)%5;
                    break;
                }
                else if ((k == (value.size()-1)) && (s == 4))
                {
                    token[s].append (value, k, 1);
                    i = k+1;
                    break;
                }
                else
                {
                    token[s].append (value, k, 1);
                }
            }
        }
        if (token[BATCH_ITEM_ID_COL].size() && first == 0)
        {
            first = 1;
            if (atoi (token[BATCH_STACK_COL].c_str()) == 0)
                init = 0;
            else
                init = 0;
        }
        if (token[BATCH_ITEM_ID_COL].size())
        {
            if (atoi (token[BATCH_STACK_COL].c_str()) > n)
            {
                n = atoi (token[BATCH_STACK_COL].c_str());
                stack_nbr++;
            }
            createItem(items, token, p++);
        }
        for (k = 0; k < BATCH_COL_LENGTH; k++)
        {
            token[k].clear ();
        }
        s = 0;
    }
    // Create stacks.
    stacks = new GlassStack[stack_nbr];
    cout << "\tStacks: " << stack_nbr << endl;
    log_file << "\tStacks: " << stack_nbr << endl;
    j = 0, k = 0;
    // Loop on batch's stacks number.
    for (i = 0; i < stack_nbr; i++)
    {
        stacks[i].Setstack_id(i + init);
        // compute each stack's items number.
        for (j = k; j < batch_items; j++)
        {
            if (items[j].Getitem_stack() == stacks[i].Getstack_id())
            {
                stacks[i].Increaseitem_nbr();
            }
            else
            {
                k = j;
                break;
            }
        }
        // allocate memory for stacks's items.
        stacks[i].item = new GlassItem[stacks[i].Getitem_nbr()];
    }
    j = 0, k = 0;
    // Loop on batch's stacks and set each stack's items.
    for (i = 0; i < stack_nbr; i++)
    {
        for (j = k; j < batch_items; j++)
        {
            if (items[j].Getitem_stack() == stacks[i].Getstack_id())
            {
                stacks[i].Setitem(items[j]);
            }
            else
            {
                k = j;
                break;
            }
        }
    }
    filess.close();
    cout << "\t--- Batch file parsed successfully ---" << endl;
    log_file << "\t--- Batch file parsed successfully ---" << endl;
}


/**
 * This function parse solution csv file to obtain
 * the user algorithm result into a node array.
 * path: input / path to solution file.
**/
void parseSolution (string path)
{
    GlassNode *node;
    string value, temp;
    string token[SOLUTION_COL_LENGTH];
    unsigned int i, k, s = 0, n = 0;
    int p = 0;
    unsigned int plate_id = 0;
    ifstream filess (path.c_str());
    if (!filess.is_open())
    {
        opened_file = false;
        cout << endl << "\t--- Unable to Open Solution file / file not found ---" << endl << "\tSolution file path: " << path << endl;
        log_file << endl << "\t--- Unable to Open Solution file / file not found ---" << endl << "\tSolution file path: " << path << endl;
        cout <<     "\t--- Solution file not parsed ---" << endl;
        log_file << "\t--- Solution file not parsed ---" << endl;
        return;
    }
    cout << endl << "\t--- Parsing Solution file ---" << endl << "\tSolution file path: " << path << endl;
    log_file << endl << "\t--- Parsing Solution file ---" << endl << "\tSolution file path: " << path << endl;
    // Get plates list.
    getline (filess, value, '\n');
    // Get the first plate ID.
    getline (filess, value, '\n');
    lines_nbr++;
    temp = value.substr(0, value.find(';'));
    plate_id = atoi (temp.c_str());
    plates_list[plates_nbr][0] = plate_id;
    plates_list[plates_nbr][1] = -1;
    plates_nbr +=1;
    while (filess.good())
    {
        getline (filess, value, '\n');
        lines_nbr++;
        temp = value.substr(0, value.find(';'));
        plate_id = atoi (temp.c_str());
        // Loop on solution used plates number.
        for (i = 0; i < plates_nbr; i++){
            // If plate is already counted, increase plate's children number.
            if (plate_id == plates_list[i][0])
            {
                plates_list[i][1]++;
                break;
            }
        }
        // If plate_id is not counted, increase plates number.
        if (i == plates_nbr)
        {
            plates_list[plates_nbr][0] = plate_id;
            plates_nbr +=1;
            plates_list[plates_nbr][1] = 0;
        }
    }
    node_nbr = (lines_nbr - plates_nbr - 1);
    plate = new GlassPlate[plates_nbr];
    node = new GlassNode[node_nbr];
    filess.clear ();
    filess.seekg (0, filess.beg);
    // Get rid of fields name line.
    getline (filess, value, '\n');
    while (filess.good())
    {
        getline (filess, value, '\n');
        // Loop on value string to split solution column tokens.
        for (i = 0; i < value.size();)
        {
            for (k = i; k < value.size(); k++)
            {
                if(value[k] == ';')
                {
                    i = k+1;
                    s = (s+1)%9;
                    break;
                }
                else if (k == (value.size()-1) && (s == 8))
                {
                    token[s].append (value, k, 1);
                    i = k+1;
                    break;
                }
                else
                {
                    token[s].append (value, k, 1);
                }
            }
        }
        if (token [SOLUTION_PLATE_ID_COL].size() != 0)
        {
            if (token[SOLUTION_PARENT_COL].size() == 0) // Plate.
            {
                if (atoi(token[SOLUTION_PLATE_ID_COL].c_str()) == p)
                {
                    createPlate (token, p++);
                }
                else
                {
                    cout << "\tPlate utilization Sequence Constraint violated ";
                    cout << "\t(ERROR: Plate " << atoi(token[SOLUTION_PLATE_ID_COL].c_str())<< " is used when Plate " << p << " is expected)" << endl;
                    log_file << "\tPlate utilization Sequence Constraint violated ";
                    log_file << "\t(ERROR: Plate " << atoi(token[SOLUTION_PLATE_ID_COL].c_str())<< " is used when Plate " << p << " is expected)" << endl;
                    constraint_error |= PARSE_ERROR_MASK;
                }
            }
            else // Piece.
            {
                createNode (node, token ,n++);
                max_cut_stage = ( max_cut_stage < atoi (token[SOLUTION_CUT_COL].c_str()) ) ? atoi (token[SOLUTION_CUT_COL].c_str()) : max_cut_stage;
                switch (atoi (token[SOLUTION_TYPE_COL].c_str()))
                {
                case -3:
                    residual_node++;
                    break;
                case -2:
                    branch_node++;
                    break;
                case -1:
                    waste_node++;
                    break;
                default:
                    useful_node++;
                    break;
                }
            }
        }
        for (k = 0; k < SOLUTION_COL_LENGTH; k++)
        {
            token[k].clear ();
        }
        s = 0;
    }
    cout << "\tPlates: " << plates_nbr ;
    log_file << "\tPlates: " << plates_nbr ;
    if (plates_nbr > plate_nbr_limit)
    {
        cout << " (ERROR: Exceed plates number limit which's " << plate_nbr_limit << ")" << endl;
        log_file << " (ERROR: Exceed plates number limit which's " << plate_nbr_limit << ")" << endl;
        constraint_error |= PARSE_ERROR_MASK;
    }
    else
    {
        cout << endl;
        log_file << endl;
    }
    // build tree structure.
    buildDataStructure (node);
    cout << "\tNodes: " << (lines_nbr - 1);
    log_file << "\tNodes: " << (lines_nbr - 1);
    filess.close();
    cout << endl;
    log_file << endl;
    cout << "\tbranches: " << branch_node + plates_nbr << endl << "\tItems: " << useful_node << endl << "\tResiduals: " << residual_node;
    log_file << "\tbranches: " << branch_node << endl << "\tItems: " << useful_node << endl << "\tResiduals: " << residual_node;
    if (residual_node > 1)
    {
        cout << " (ERROR: Solution should have one and only one residual item located on the rightmost node of the last plate)";
        log_file << " (ERROR: Solution should have one and only one residual item located on the rightmost node of the last plate)";
        constraint_error |= PARSE_ERROR_MASK;
    }
    cout << endl << "\tWaste: " << waste_node;
    log_file << endl << "\tWaste: " << waste_node;
    cout << endl << "\tUsed Cut Stages: " << max_cut_stage;
    log_file << endl << "\tUsed Cut Stages: " << max_cut_stage;
    if (max_cut_stage > 4)
    {
        cout << " (ERROR: Solution Algorithm has " << max_cut_stage << " cut stages, while the max is 4 stages)" << endl;
        log_file << " (ERROR: Solution Algorithm has " << max_cut_stage << " cut stages, while the max is 4 stages)" << endl;
        constraint_error |= PARSE_ERROR_MASK;
    }
    cout << endl; // << " waste: " << node_nbr - (useful_node + branch_node) << endl;
    log_file << endl;
    if (constraint_error > 0)
    {
        cout << "\tTree structure built" << endl;
        log_file << "\tTree structure built" << endl;
        cout << "\t--- ERROR -- Solution file Contains an INVALID SOLUTION ---" << endl;
        log_file << "\t--- ERROR -- Solution file Contains an INVALID SOLUTION ---" << endl;
    }
    else
    {
        cout << "\tTree structure built" << endl;
        log_file << "\tTree structure built" << endl;
    }
    cout << "\t--- Solution file parsed successfully ---" << endl;
    log_file << "\t--- Solution file parsed successfully ---" << endl;
}



/* ==================================================================================== */
/*                          Tree structure Build functions                              */
/* ==================================================================================== */
/**
 * This function takes the node array given by the solution parser
 * and build a tree structure of plates and their successors
 * to build the first level of the tree structure.
 * node: input / all solution nodes as a GlassNode array.
**/
void buildDataStructure (GlassNode *node)
{
    unsigned int i, j, index = 0;
    int waste = 0.0, useful = 0.0;
    unsigned int c_nbr; // maintains plate[i] nodes number.
    sol_items = new GlassNode[useful_node];
    // Loop on plates list.
    for (i = 0; i < plates_nbr; i++)
    {
        useful = 0.0;
        waste = 0.0;
        c_nbr = plate[i].Getnodes_nbr ();
        // if first plate, start at index 0 in node array. if not, start read at index = plate[i-1].nodes_nbr.
        index += (i > 0) ? plate[i-1].Getnodes_nbr () : 0;
        // Loop on plate node list (not only its successors).
        for (j = 0; j < c_nbr; j++)
        {
            if (node[index+j].Getparent() == plate[i].Getnode_id())
            {
                switch (node[index+j].Gettype())
                {
                case -3: // residual.
                    if (node[index+j].Getplate_id() == (plates_nbr - 1))
                    {
                        if (j == c_nbr-1)
                        {
                            cout << "\tNode " << node[index+j].Getnode_id() << ": is a residual" << endl;
                            log_file << "\tNode " << node[index+j].Getnode_id() << ": is a residual" << endl;
                            plate[node[index+j].Getplate_id()].Setresidual (node[index+j]);
                        }
                        else
                        {
                            cout << "\tERROR -- Node " << node[index+j].Getnode_id() << ": is a residual (ERROR: Residual item should be the rightmost node of the last plate)" << endl;
                            log_file << "\tERROR -- Node " << node[index+j].Getnode_id() << ": is a residual (ERROR: Residual item should be the rightmost node of the last plate)" << endl;
                            constraint_error |= PARSE_ERROR_MASK;
                        }
                    }
                    else
                    {
                        cout << "\tERROR -- Node " << node[index+j].Getnode_id() << ": is a residual (ERROR: Residual item can be located only on the last plate)" << endl;
                        log_file << "\tERROR -- Node " << node[index+j].Getnode_id() << ": is a residual (ERROR: Residual item can be located only on the last plate)" << endl;
                        constraint_error |= PARSE_ERROR_MASK;
                    }
                    break;
                case -2: // branch.
                    checkSuccessors (node, sol_items, i, node[index+j], index + c_nbr, index+j+1);
                    break;
                case -1: // waste.
                    if ((node[index+j].Getplate_id() == (plates_nbr - 1)) && (j == c_nbr-1))
                    {
                        cout << "\tNode " << node[index+j].Getnode_id() << ": is a WASTE that should be considered as a RESIDUAL" << endl;
                        log_file << "\tNode " << node[index+j].Getnode_id() << ": is a WASTE that should be considered as a RESIDUAL" << endl;
                        residual_node++;
                        waste_node--;
                        plate[node[index+j].Getplate_id()].Setresidual (node[index+j]);
                    }
                    else
                    {
                        waste = (node[index+j].Getwidth() * node[index+j].Getheight());
                        waste += plate[i].Getwaste();
                        plate[i].Setwaste(waste);
                    }
                    break;
                default: // final piece.
                    sol_items[s_idx++] = node [index+j];
                    useful = (node[index+j].Getwidth() * node[index+j].Getheight());
                    useful += plate[i].Getuseful();
                    plate[i].Setuseful(useful);
                    break;
                }
                plate[i].Setsuccessor(node[index+j]);
            }
        }
    }
}


/**
 * This function, for each node of a plate, found its successors
 * recursively to build the rest of the tree.
 * node: input / node array.
 * sol_items: output / will contain solution items (final items)
 * plate_id: input / plate index
 * parent: in-out / the parent node looking for its successors
 * c_end: input / end index in node array
 * index: input / start index in node array
**/
void checkSuccessors (GlassNode *node, GlassNode *sol_items, int plate_id, GlassNode &parent, int c_end, int index)
{
    int i, nbr = 0;
    int waste = 0.0 , useful = 0.0;
    // Compute node successors number.
    for (i = index; i < c_end; i++)
        if (node[i].Getparent() == parent.Getnode_id())
            nbr++;
    // Allocate node successors array.
    parent.successor = new GlassNode[nbr];
    for (i = index; i < c_end; i++)
    {
        if (node[i].Getparent() == parent.Getnode_id())
        {
            // if Setsuccessor function call returned 1.
            if (parent.Setsuccessor(node[i]) == 1)
            {
                cout << "\tERROR -- Node: " << parent.Getnode_id() << " is " << parent.Getcut() << "-cut and has more than 2 successors" << endl;
                log_file << "\tERROR -- Node: " << parent.Getnode_id() << " is " << parent.Getcut() << "-cut and has more than 2 successors" << endl;
                constraint_error |= PARSE_ERROR_MASK;
            }
            switch (node[i].Gettype())
            {
            case -3: // residual.
                cout << "\tERROR -- Node " << node[i].Getnode_id() << ": is a residual (ERROR: residual must be 1-cut node)" << endl;
                log_file << "\tERROR -- Node " << node[i].Getnode_id() << ": is a residual (ERROR: residual must be 1-cut node)" << endl;
                constraint_error |= PARSE_ERROR_MASK;
                break;
            case -2: // branch.
                checkSuccessors (node, sol_items, plate_id, parent.successor[parent.Getsuccessor_nbr()-1], c_end, i+1);
                break;
            case -1: // waste.
                waste = (node[i].Getwidth() * node[i].Getheight());
                waste += plate[plate_id].Getwaste();
                plate[plate_id].Setwaste(waste);
                break;
            default: // final piece.
                sol_items[s_idx++] = node[i];
                useful = (node[i].Getwidth() * node[i].Getheight());
                useful += plate[plate_id].Getuseful();
                plate[plate_id].Setuseful(useful);
                break;
            }
        }
    }
}



/* ==================================================================================== */
/*              Different Class instantiation and initialization functions              */
/* ==================================================================================== */
/**
 * This function creates a plate at a given index of the
 * plate array used by the user solution.
 * token: input / string array contains plate parameters.
 * p: input / index at plate array
**/
void createPlate (string *token, int p)
{
    plate[p].Setplate_id (atoi (token[SOLUTION_PLATE_ID_COL].c_str()));
    plate[p].Setnode_id (atoi (token[SOLUTION_NODE_ID_COL].c_str()));
    plate[p].Setpos_x (atoi (token[SOLUTION_X_COL].c_str()));
    plate[p].Setpos_y (atoi (token[SOLUTION_Y_COL].c_str()));
    // if solution read value of plate length value is different from plate length optimization parameter value
    if (atoi (token[SOLUTION_WIDTH_COL].c_str()) != plate_w)
    {
        cout << "\tERROR --  Plate " << atoi (token[SOLUTION_PLATE_ID_COL].c_str()) << ": width not equal to widthPlates " << plate_w << endl;
        log_file << "\tERROR --  Plate " << atoi (token[SOLUTION_PLATE_ID_COL].c_str()) << ": width not equal to widthPlates " << plate_w << endl;
        constraint_error |= PARSE_ERROR_MASK;
    }
    plate[p].Setwidth (atoi (token[SOLUTION_WIDTH_COL].c_str()));
    // if solution read value of plate width value is different from plate height optimization parameter value
    if (atoi (token[SOLUTION_HEIGHT_COL].c_str()) != plate_h)
    {
        cout << "\tERROR -- Plate " << atoi (token[SOLUTION_PLATE_ID_COL].c_str()) << ": height not equal to lengthPlates " << plate_h << endl;
        log_file << "\tERROR -- Plate " << atoi (token[SOLUTION_PLATE_ID_COL].c_str()) << ": height not equal to lengthPlates " << plate_h << endl;
        constraint_error |= PARSE_ERROR_MASK;
    }
    plate[p].Setheight (atoi (token[SOLUTION_HEIGHT_COL].c_str()));
    plate[p].Settype (atoi (token[SOLUTION_TYPE_COL].c_str()));
    plate[p].Setcut (atoi (token[SOLUTION_CUT_COL].c_str()));
    plate[p].successor = new GlassNode[plates_list[p][1]];
    plate[p].Setnodes_nbr(plates_list[p][1]);
    plate[p].Setwaste(0);
}


/**
 * This function creates a node at a given index of the
 * node array used by the user solution.
 * node: output / list in which function will create the node
 * token: input / string array contains node parameters.
 * n: input / index at node array
**/
void createNode (GlassNode *node, string *token, int n)
{
    node[n].Setplate_id (atoi (token[SOLUTION_PLATE_ID_COL].c_str()));
    node[n].Setnode_id (atoi (token[SOLUTION_NODE_ID_COL].c_str()));
    node[n].Setpos_x (atoi (token[SOLUTION_X_COL].c_str()));
    node[n].Setpos_y (atoi (token[SOLUTION_Y_COL].c_str()));
    node[n].Setwidth (atoi (token[SOLUTION_WIDTH_COL].c_str()));
    node[n].Setheight (atoi (token[SOLUTION_HEIGHT_COL].c_str()));
    node[n].Settype (atoi (token[SOLUTION_TYPE_COL].c_str()));
    node[n].Setcut (atoi (token[SOLUTION_CUT_COL].c_str()));
    node[n].Setparent (atoi (token[SOLUTION_PARENT_COL].c_str()));
}


/**
 * This function takes the node array given by the solution parser
 * and build a tree structure of plates and their successors
 * to build the first level of the tree structure.
 * node: input / node array.
**/
void createItem (GlassItem *items, string *token, int p)
{
    items[p].Setitem_id (atoi (token[BATCH_ITEM_ID_COL].c_str()));
    items[p].Setitem_h (atoi (token[BATCH_LENGTH_ITEM_COL].c_str()));
    items[p].Setitem_w (atoi (token[BATCH_WIDTH_ITEM_COL].c_str()));
    items[p].Setitem_stack (atoi (token[BATCH_STACK_COL].c_str()));
    items[p].Setitem_seq (atoi (token[BATCH_SEQ_COL].c_str()));
}
