/**
 * @file professional_workflow_data.cpp
 * @brief Professional workflow test data fixtures
 */

#include "professional_workflow_data.h"

namespace test_fixtures {
namespace professional_workflow {

const std::vector<WorkflowTestCase> WORKFLOW_TEST_CASES = {
    {
        "File Analysis Workflow",
        "Load ETI file -> Analyze services -> Navigate frames -> Export results",
        3000,  // 3 second timeout
        true   // should_succeed
    },
    {
        "Real-time Analysis Workflow", 
        "Enable real-time -> Configure network -> Process stream -> Monitor quality",
        5000,  // 5 second timeout
        true   // should_succeed
    },
    {
        "Error Detection Workflow",
        "Load problematic file -> Detect errors -> Navigate to problems -> Generate report",
        4000,  // 4 second timeout
        true   // should_succeed
    }
};

const std::vector<UITestScenario> UI_TEST_SCENARIOS = {
    {
        "Three Panel Layout Test",
        {"Explorer Panel", "Main Content", "Properties Panel"},
        true,  // panels_visible
        {25, 50, 25}  // expected_proportions
    },
    {
        "Service Selection Test",
        {"Service Tree", "Properties Display", "Frame Filter"},
        true,  // panels_visible
        {30, 40, 30}  // expected_proportions
    }
};

} // namespace professional_workflow
} // namespace test_fixtures