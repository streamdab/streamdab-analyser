/**
 * @file professional_workflow_data.h
 * @brief Professional workflow test data fixtures
 */

#pragma once

#include <vector>
#include <string>

struct WorkflowTestCase {
    std::string name;
    std::string description;
    int timeout_ms;
    bool should_succeed;
};

struct UITestScenario {
    std::string name;
    std::vector<std::string> required_panels;
    bool panels_visible;
    std::vector<int> expected_proportions;
};

namespace test_fixtures {
namespace professional_workflow {

extern const std::vector<WorkflowTestCase> WORKFLOW_TEST_CASES;
extern const std::vector<UITestScenario> UI_TEST_SCENARIOS;

} // namespace professional_workflow
} // namespace test_fixtures