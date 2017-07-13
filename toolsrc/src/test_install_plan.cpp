#include "CppUnitTest.h"
#include "vcpkg_Dependencies.h"
#include "vcpkg_Util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace vcpkg;

namespace UnitTest1
{
    class InstallPlanTests : public TestClass<InstallPlanTests>
    {
        struct PackageSpecMap
        {
            std::unordered_map<PackageSpec, SourceControlFile> map;
            PackageSpec get_package_spec(std::vector<std::unordered_map<std::string, std::string>>&& fields)
            {
                auto m_pgh = vcpkg::SourceControlFile::parse_control_file(std::move(fields));
                Assert::IsTrue(m_pgh.has_value());
                auto& scf = *m_pgh.get();

                auto spec = PackageSpec::from_name_and_triplet(scf->core_paragraph->name, Triplet::X86_WINDOWS);
                Assert::IsTrue(spec.has_value());
                map.emplace(*spec.get(), std::move(*scf.get()));
                return PackageSpec{*spec.get()};
            }
        };

        static void features_check(Dependencies::AnyAction* install_action,
                                   std::string pkg_name,
                                   std::vector<std::string> vec)
        {
            const auto& plan = *install_action->install_plan.get();
            const auto& feature_list = plan.feature_list;

            Assert::AreEqual(pkg_name.c_str(),
                             (*plan.any_paragraph.source_control_file.get())->core_paragraph->name.c_str());
            Assert::AreEqual(size_t(vec.size()), feature_list.size());

            for (auto&& feature_name : vec)
            {
                if (feature_name == "core" || feature_name == "")
                {
                    Assert::IsTrue(Util::find(feature_list, "core") != feature_list.end() ||
                                   Util::find(feature_list, "") != feature_list.end());
                    continue;
                }
                Assert::IsTrue(Util::find(feature_list, feature_name) != feature_list.end());
            }
        }

        static void remove_plan_check(Dependencies::AnyAction* install_action, std::string pkg_name)
        {
            Assert::AreEqual(pkg_name.c_str(), install_action->remove_plan.get()->spec.name().c_str());
        }

        TEST_METHOD(basic_install_scheme)
        {
            std::vector<std::unique_ptr<StatusParagraph>> status_paragraphs;

            PackageSpecMap spec_map;

            auto spec_a = spec_map.get_package_spec({{{"Source", "a"}, {"Version", "1.2.8"}, {"Build-Depends", "b"}}});
            auto spec_b = spec_map.get_package_spec({{{"Source", "b"}, {"Version", "1.3"}, {"Build-Depends", "c"}}});
            auto spec_c = spec_map.get_package_spec({{{"Source", "c"}, {"Version", "2.5.3"}, {"Build-Depends", ""}}});

            auto map_port = Dependencies::MapPortFile(spec_map.map);
            auto install_plan =
                Dependencies::create_install_plan(map_port, {spec_a}, StatusParagraphs(std::move(status_paragraphs)));

            Assert::AreEqual(size_t(3), install_plan.size());
            Assert::AreEqual("c", install_plan[0].spec.name().c_str());
            Assert::AreEqual("b", install_plan[1].spec.name().c_str());
            Assert::AreEqual("a", install_plan[2].spec.name().c_str());
        }

        TEST_METHOD(multiple_install_scheme)
        {
            std::vector<std::unique_ptr<StatusParagraph>> status_paragraphs;

            PackageSpecMap spec_map;

            auto spec_a = spec_map.get_package_spec({{{"Source", "a"}, {"Version", "1.2.8"}, {"Build-Depends", "d"}}});
            auto spec_b = spec_map.get_package_spec({{{"Source", "b"}, {"Version", "1.3"}, {"Build-Depends", "d, e"}}});
            auto spec_c =
                spec_map.get_package_spec({{{"Source", "c"}, {"Version", "2.5.3"}, {"Build-Depends", "e, h"}}});
            auto spec_d =
                spec_map.get_package_spec({{{"Source", "d"}, {"Version", "4.0"}, {"Build-Depends", "f, g, h"}}});
            auto spec_e = spec_map.get_package_spec({{{"Source", "e"}, {"Version", "1.0"}, {"Build-Depends", "g"}}});
            auto spec_f = spec_map.get_package_spec({{{"Source", "f"}, {"Version", "1.0"}, {"Build-Depends", ""}}});
            auto spec_g = spec_map.get_package_spec({{{"Source", "g"}, {"Version", "1.0"}, {"Build-Depends", ""}}});
            auto spec_h = spec_map.get_package_spec({{{"Source", "h"}, {"Version", "1.0"}, {"Build-Depends", ""}}});

            auto map_port = Dependencies::MapPortFile(spec_map.map);
            auto install_plan = Dependencies::create_install_plan(
                map_port, {spec_a, spec_b, spec_c}, StatusParagraphs(std::move(status_paragraphs)));

            auto iterator_pos = [&](const PackageSpec& spec) -> int {
                auto it = std::find_if(
                    install_plan.begin(), install_plan.end(), [&](auto& action) { return action.spec == spec; });
                Assert::IsTrue(it != install_plan.end());
                return (int)(it - install_plan.begin());
            };

            int a_pos = iterator_pos(spec_a), b_pos = iterator_pos(spec_b), c_pos = iterator_pos(spec_c),
                d_pos = iterator_pos(spec_d), e_pos = iterator_pos(spec_e), f_pos = iterator_pos(spec_f),
                g_pos = iterator_pos(spec_g), h_pos = iterator_pos(spec_h);

            Assert::IsTrue(a_pos > d_pos);
            Assert::IsTrue(b_pos > e_pos);
            Assert::IsTrue(b_pos > d_pos);
            Assert::IsTrue(c_pos > e_pos);
            Assert::IsTrue(c_pos > h_pos);
            Assert::IsTrue(d_pos > f_pos);
            Assert::IsTrue(d_pos > g_pos);
            Assert::IsTrue(d_pos > h_pos);
            Assert::IsTrue(e_pos > g_pos);
        }

        TEST_METHOD(long_install_scheme)
        {
            using Pgh = std::unordered_map<std::string, std::string>;
            std::vector<std::unique_ptr<StatusParagraph>> status_paragraphs;
            status_paragraphs.push_back(std::make_unique<StatusParagraph>(Pgh{{"Package", "j"},
                                                                              {"Version", "1.2.8"},
                                                                              {"Architecture", "x86-windows"},
                                                                              {"Multi-Arch", "same"},
                                                                              {"Depends", "k"},
                                                                              {"Status", "install ok installed"}}));
            status_paragraphs.push_back(std::make_unique<StatusParagraph>(Pgh{{"Package", "k"},
                                                                              {"Version", "1.2.8"},
                                                                              {"Architecture", "x86-windows"},
                                                                              {"Multi-Arch", "same"},
                                                                              {"Depends", ""},
                                                                              {"Status", "install ok installed"}}));

            PackageSpecMap spec_map;

            auto spec_h =
                spec_map.get_package_spec({{{"Source", "h"}, {"Version", "1.2.8"}, {"Build-Depends", "j, k"}}});
            auto spec_c = spec_map.get_package_spec(
                {{{"Source", "c"}, {"Version", "1.2.8"}, {"Build-Depends", "d, e, f, g, h, j, k"}}});
            auto spec_k = spec_map.get_package_spec({{{"Source", "k"}, {"Version", "1.2.8"}, {"Build-Depends", ""}}});
            auto spec_b = spec_map.get_package_spec(
                {{{"Source", "b"}, {"Version", "1.2.8"}, {"Build-Depends", "c, d, e, f, g, h, j, k"}}});
            auto spec_d = spec_map.get_package_spec(
                {{{"Source", "d"}, {"Version", "1.2.8"}, {"Build-Depends", "e, f, g, h, j, k"}}});
            auto spec_j = spec_map.get_package_spec({{{"Source", "j"}, {"Version", "1.2.8"}, {"Build-Depends", "k"}}});
            auto spec_f =
                spec_map.get_package_spec({{{"Source", "f"}, {"Version", "1.2.8"}, {"Build-Depends", "g, h, j, k"}}});
            auto spec_e = spec_map.get_package_spec(
                {{{"Source", "e"}, {"Version", "1.2.8"}, {"Build-Depends", "f, g, h, j, k"}}});
            auto spec_a = spec_map.get_package_spec(
                {{{"Source", "a"}, {"Version", "1.2.8"}, {"Build-Depends", "b, c, d, e, f, g, h, j, k"}}});
            auto spec_g =
                spec_map.get_package_spec({{{"Source", "g"}, {"Version", "1.2.8"}, {"Build-Depends", "h, j, k"}}});

            auto map_port = Dependencies::MapPortFile(spec_map.map);
            auto install_plan =
                Dependencies::create_install_plan(map_port, {spec_a}, StatusParagraphs(std::move(status_paragraphs)));

            Assert::AreEqual(size_t(8), install_plan.size());
            Assert::AreEqual("h", install_plan[0].spec.name().c_str());
            Assert::AreEqual("g", install_plan[1].spec.name().c_str());
            Assert::AreEqual("f", install_plan[2].spec.name().c_str());
            Assert::AreEqual("e", install_plan[3].spec.name().c_str());
            Assert::AreEqual("d", install_plan[4].spec.name().c_str());
            Assert::AreEqual("c", install_plan[5].spec.name().c_str());
            Assert::AreEqual("b", install_plan[6].spec.name().c_str());
            Assert::AreEqual("a", install_plan[7].spec.name().c_str());
        }

        TEST_METHOD(basic_feature_test_1)
        {
            using Pgh = std::unordered_map<std::string, std::string>;

            std::vector<std::unique_ptr<StatusParagraph>> status_paragraphs;
            status_paragraphs.push_back(std::make_unique<StatusParagraph>(Pgh{{"Package", "a"},
                                                                              {"Default-Features", ""},
                                                                              {"Version", "1.3.8"},
                                                                              {"Architecture", "x86-windows"},
                                                                              {"Multi-Arch", "same"},
                                                                              {"Depends", "b, b[beefeatureone]"},
                                                                              {"Status", "install ok installed"}}));
            status_paragraphs.push_back(std::make_unique<StatusParagraph>(Pgh{{"Package", "b"},
                                                                              {"Feature", "beefeatureone"},
                                                                              {"Architecture", "x86-windows"},
                                                                              {"Multi-Arch", "same"},
                                                                              {"Depends", ""},
                                                                              {"Status", "install ok installed"}}));
            status_paragraphs.push_back(std::make_unique<StatusParagraph>(Pgh{{"Package", "b"},
                                                                              {"Default-Features", "beefeatureone"},
                                                                              {"Version", "1.3"},
                                                                              {"Architecture", "x86-windows"},
                                                                              {"Multi-Arch", "same"},
                                                                              {"Depends", ""},
                                                                              {"Status", "install ok installed"}}));

            PackageSpecMap spec_map;

            auto spec_a =
                FullPackageSpec{spec_map.get_package_spec(
                                    {{{"Source", "a"}, {"Version", "1.3.8"}, {"Build-Depends", "b, b[beefeatureone]"}},
                                     {{"Feature", "featureone"},
                                      {"Description", "the first feature for a"},
                                      {"Build-Depends", "b[beefeaturetwo]"}}

                                    }),
                                {"featureone"}};
            auto spec_b = FullPackageSpec{spec_map.get_package_spec(
                {{{"Source", "b"}, {"Version", "1.3"}, {"Build-Depends", ""}},
                 {{"Feature", "beefeatureone"}, {"Description", "the first feature for b"}, {"Build-Depends", ""}},
                 {{"Feature", "beefeaturetwo"}, {"Description", "the second feature for b"}, {"Build-Depends", ""}},
                 {{"Feature", "beefeaturethree"}, {"Description", "the third feature for b"}, {"Build-Depends", ""}}

                })};

            auto install_plan = Dependencies::create_feature_install_plan(
                spec_map.map, {spec_a}, StatusParagraphs(std::move(status_paragraphs)));

            Assert::AreEqual(size_t(4), install_plan.size());
            remove_plan_check(&install_plan[0], "a");
            remove_plan_check(&install_plan[1], "b");
            features_check(&install_plan[2], "b", {"beefeatureone", "core", "beefeatureone"});
            features_check(&install_plan[3], "a", {"featureone", "core"});
        }

        TEST_METHOD(basic_feature_test_2)
        {
            using Pgh = std::unordered_map<std::string, std::string>;

            std::vector<std::unique_ptr<StatusParagraph>> status_paragraphs;

            PackageSpecMap spec_map;

            auto spec_a =
                FullPackageSpec{spec_map.get_package_spec(
                                    {{{"Source", "a"}, {"Version", "1.3.8"}, {"Build-Depends", "b[beefeatureone]"}},
                                     {{"Feature", "featureone"},
                                      {"Description", "the first feature for a"},
                                      {"Build-Depends", "b[beefeaturetwo]"}}

                                    }),
                                {"featureone"}};
            auto spec_b = FullPackageSpec{spec_map.get_package_spec(
                {{{"Source", "b"}, {"Version", "1.3"}, {"Build-Depends", ""}},
                 {{"Feature", "beefeatureone"}, {"Description", "the first feature for b"}, {"Build-Depends", ""}},
                 {{"Feature", "beefeaturetwo"}, {"Description", "the second feature for b"}, {"Build-Depends", ""}},
                 {{"Feature", "beefeaturethree"}, {"Description", "the third feature for b"}, {"Build-Depends", ""}}

                })};

            auto install_plan = Dependencies::create_feature_install_plan(
                spec_map.map, {spec_a}, StatusParagraphs(std::move(status_paragraphs)));

            Assert::AreEqual(size_t(2), install_plan.size());
            features_check(&install_plan[0], "b", {"beefeatureone", "beefeaturetwo", "core"});
            features_check(&install_plan[1], "a", {"featureone", "core"});
        }

        TEST_METHOD(basic_feature_test_3)
        {
            using Pgh = std::unordered_map<std::string, std::string>;

            std::vector<std::unique_ptr<StatusParagraph>> status_paragraphs;
            status_paragraphs.push_back(std::make_unique<StatusParagraph>(Pgh{{"Package", "a"},
                                                                              {"Default-Features", ""},
                                                                              {"Version", "1.3"},
                                                                              {"Architecture", "x86-windows"},
                                                                              {"Multi-Arch", "same"},
                                                                              {"Depends", ""},
                                                                              {"Status", "install ok installed"}}));

            PackageSpecMap spec_map;

            auto spec_a = FullPackageSpec{
                spec_map.get_package_spec(
                    {{{"Source", "a"}, {"Version", "1.3"}, {"Build-Depends", "b"}},
                     {{"Feature", "one"}, {"Description", "the first feature for a"}, {"Build-Depends", ""}}}),
                {"core"}};
            auto spec_b = FullPackageSpec{spec_map.get_package_spec({
                {{"Source", "b"}, {"Version", "1.3"}, {"Build-Depends", ""}},
            })};
            auto spec_c = FullPackageSpec{spec_map.get_package_spec({
                                              {{"Source", "c"}, {"Version", "1.3"}, {"Build-Depends", "a[one]"}},
                                          }),
                                          {"core"}};

            auto install_plan = Dependencies::create_feature_install_plan(
                spec_map.map, {spec_c, spec_a}, StatusParagraphs(std::move(status_paragraphs)));

            Assert::AreEqual(size_t(4), install_plan.size());
            remove_plan_check(&install_plan[0], "a");
            features_check(&install_plan[1], "b", {"core"});
            features_check(&install_plan[2], "a", {"one", "core"});
            features_check(&install_plan[3], "c", {"core"});
        }

        TEST_METHOD(basic_feature_test_4)
        {
            using Pgh = std::unordered_map<std::string, std::string>;

            std::vector<std::unique_ptr<StatusParagraph>> status_paragraphs;
            status_paragraphs.push_back(std::make_unique<StatusParagraph>(Pgh{{"Package", "a"},
                                                                              {"Default-Features", ""},
                                                                              {"Version", "1.3"},
                                                                              {"Architecture", "x86-windows"},
                                                                              {"Multi-Arch", "same"},
                                                                              {"Depends", ""},
                                                                              {"Status", "install ok installed"}}));
            status_paragraphs.push_back(std::make_unique<StatusParagraph>(Pgh{{"Package", "a"},
                                                                              {"Feature", "one"},
                                                                              {"Architecture", "x86-windows"},
                                                                              {"Multi-Arch", "same"},
                                                                              {"Depends", ""},
                                                                              {"Status", "install ok installed"}}));

            PackageSpecMap spec_map;

            auto spec_a = FullPackageSpec{
                spec_map.get_package_spec(
                    {{{"Source", "a"}, {"Version", "1.3"}, {"Build-Depends", "b"}},
                     {{"Feature", "one"}, {"Description", "the first feature for a"}, {"Build-Depends", ""}}}),
            };
            auto spec_b = FullPackageSpec{spec_map.get_package_spec({
                {{"Source", "b"}, {"Version", "1.3"}, {"Build-Depends", ""}},
            })};
            auto spec_c = FullPackageSpec{spec_map.get_package_spec({
                                              {{"Source", "c"}, {"Version", "1.3"}, {"Build-Depends", "a[one]"}},
                                          }),
                                          {"core"}};

            auto install_plan = Dependencies::create_feature_install_plan(
                spec_map.map, {spec_c}, StatusParagraphs(std::move(status_paragraphs)));

            Assert::AreEqual(size_t(1), install_plan.size());
            features_check(&install_plan[0], "c", {"core"});
        }

        TEST_METHOD(basic_feature_test_5)
        {
            using Pgh = std::unordered_map<std::string, std::string>;

            std::vector<std::unique_ptr<StatusParagraph>> status_paragraphs;

            PackageSpecMap spec_map;

            auto spec_a = FullPackageSpec{
                spec_map.get_package_spec(
                    {{{"Source", "a"}, {"Version", "1.3"}, {"Build-Depends", ""}},
                     {{"Feature", "1"}, {"Description", "the first feature for a"}, {"Build-Depends", "b[1]"}},
                     {{"Feature", "2"}, {"Description", "the first feature for a"}, {"Build-Depends", "b[2]"}},
                     {{"Feature", "3"}, {"Description", "the first feature for a"}, {"Build-Depends", "a[2]"}}}),
                {"3"}};
            auto spec_b = FullPackageSpec{spec_map.get_package_spec({
                {{"Source", "b"}, {"Version", "1.3"}, {"Build-Depends", ""}},
                {{"Feature", "1"}, {"Description", "the first feature for a"}, {"Build-Depends", ""}},
                {{"Feature", "2"}, {"Description", "the first feature for a"}, {"Build-Depends", ""}},
            })};

            auto install_plan = Dependencies::create_feature_install_plan(
                spec_map.map, {spec_a}, StatusParagraphs(std::move(status_paragraphs)));

            Assert::AreEqual(size_t(2), install_plan.size());
            features_check(&install_plan[0], "b", {"core", "2"});
            features_check(&install_plan[1], "a", {"core", "3", "2"});
        }

        TEST_METHOD(basic_feature_test_6)
        {
            using Pgh = std::unordered_map<std::string, std::string>;

            std::vector<std::unique_ptr<StatusParagraph>> status_paragraphs;
            status_paragraphs.push_back(std::make_unique<StatusParagraph>(Pgh{{"Package", "b"},
                                                                              {"Default-Features", ""},
                                                                              {"Version", "1.3"},
                                                                              {"Architecture", "x86-windows"},
                                                                              {"Multi-Arch", "same"},
                                                                              {"Depends", ""},
                                                                              {"Status", "install ok installed"}}));
            PackageSpecMap spec_map;

            auto spec_a = FullPackageSpec{spec_map.get_package_spec({
                                              {{"Source", "a"}, {"Version", "1.3"}, {"Build-Depends", "b[core]"}},
                                          }),
                                          {"core"}};
            auto spec_b = FullPackageSpec{
                spec_map.get_package_spec({
                    {{"Source", "b"}, {"Version", "1.3"}, {"Build-Depends", ""}},
                    {{"Feature", "1"}, {"Description", "the first feature for a"}, {"Build-Depends", ""}},
                }),
                {"1"}};

            auto install_plan = Dependencies::create_feature_install_plan(
                spec_map.map, {spec_a, spec_b}, StatusParagraphs(std::move(status_paragraphs)));

            Assert::AreEqual(size_t(3), install_plan.size());
            remove_plan_check(&install_plan[0], "b");
            features_check(&install_plan[1], "b", {"core", "1"});
            features_check(&install_plan[2], "a", {"core"});
        }

        TEST_METHOD(basic_feature_test_7)
        {
            using Pgh = std::unordered_map<std::string, std::string>;

            std::vector<std::unique_ptr<StatusParagraph>> status_paragraphs;
            status_paragraphs.push_back(std::make_unique<StatusParagraph>(Pgh{{"Package", "x"},
                                                                              {"Default-Features", ""},
                                                                              {"Version", "1.3"},
                                                                              {"Architecture", "x86-windows"},
                                                                              {"Multi-Arch", "same"},
                                                                              {"Depends", "b"},
                                                                              {"Status", "install ok installed"}}));
            status_paragraphs.push_back(std::make_unique<StatusParagraph>(Pgh{{"Package", "b"},
                                                                              {"Default-Features", ""},
                                                                              {"Version", "1.3"},
                                                                              {"Architecture", "x86-windows"},
                                                                              {"Multi-Arch", "same"},
                                                                              {"Depends", ""},
                                                                              {"Status", "install ok installed"}}));
            PackageSpecMap spec_map;

            auto spec_a = FullPackageSpec{spec_map.get_package_spec({
                {{"Source", "a"}, {"Version", "1.3"}, {"Build-Depends", ""}},
            })};
            auto spec_x = FullPackageSpec{spec_map.get_package_spec({
                                              {{"Source", "x"}, {"Version", "1.3"}, {"Build-Depends", "a"}},
                                          }),
                                          {"core"}};
            auto spec_b = FullPackageSpec{
                spec_map.get_package_spec({
                    {{"Source", "b"}, {"Version", "1.3"}, {"Build-Depends", ""}, {"Default-Features", ""}},
                    {{"Feature", "1"}, {"Description", "the first feature for a"}, {"Build-Depends", ""}},
                }),
                {"1"}};

            auto install_plan = Dependencies::create_feature_install_plan(
                spec_map.map, {spec_b, spec_x}, StatusParagraphs(std::move(status_paragraphs)));

            Assert::AreEqual(size_t(5), install_plan.size());
            remove_plan_check(&install_plan[0], "x");
            remove_plan_check(&install_plan[1], "b");

            // order here may change but A < X, and B anywhere
            features_check(&install_plan[2], "a", {"core"});
            features_check(&install_plan[3], "x", {"core"});
            features_check(&install_plan[4], "b", {"core", "1"});
        }
    };
}