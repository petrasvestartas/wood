
#include "wood_xml.h"

#include "../../../stdafx.h" //go up to the folder where the CMakeLists.txt is
#include <pugixml.hpp>

namespace wood
{
namespace xml
{
std::string path_and_file_for_input_numbers = "C:\\IBOIS57\\_Code\\Software\\Python\\compas_"
                                              "wood\\net\\data\\input_numbers.xml";
std::string path_and_file_for_input_polylines = "C:\\IBOIS57\\_Code\\Software\\Python\\compas_"
                                                "wood\\net\\data\\input_polylines.xml";
std::string path_and_file_for_input_polylines_simple_case = "C:\\IBOIS57\\_Code\\Software\\Python\\compas_"
                                                            "wood\\net\\data\\input_polylines_simple_case.xml";
std::string path_and_file_for_output_polylines = "C:\\IBOIS57\\_Code\\Software\\Python\\compas_"
                                                 "wood\\net\\data\\output_polylines.xml";
std::string path_and_file_for_output_polylines_simple_case = "C:\\IBOIS57\\_Code\\Software\\Python\\compas_"
                                                             "wood\\net\\data\\output_polylines_simple_case."
                                                             "xml";

bool
file_exists_0 (const std::string &name)
{
    std::ifstream f (name.c_str ());
    return f.good ();
}

bool
read_xml_numbers (std::vector<std::vector<double> > &numbers)
{
    std::string file_path = path_and_file_for_input_numbers;
    std::string property_to_read = "input_numbers";

    printf ("%s", file_path.c_str ());
    if (!file_exists_0 (file_path))
        {
            printf ("\nread_wood::xml -> File does not exist");
            return false;
        }
    else
        {
            printf ("\read_nwood::xml -> File exists");
        }

    try
        {
            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_file (file_path.c_str ());
            if (!result)
                {
                    printf ("read_wood::xml -> CPP Wrong property, probably wrong path \n");
                    return false;
                }

            pugi::xml_node root = doc.child (property_to_read.c_str ());
            for (pugi::xml_node node : root.children ("numbers"))
                {
                    std::vector<double> numbers_list;
                    for (pugi::xml_node num_node : node.children ())
                        {
                            numbers_list.emplace_back (num_node.text ().as_double ());
                        }
                    numbers.emplace_back (numbers_list);
                }
        }
    catch (std::exception &e)
        {
            (void)e;
            printf ("read_wood::xml -> CPP Wrong property, probaby wrong "
                    "path \n");
            return false;
        }
    return true;
}

bool
read_xml_polylines (std::vector<std::vector<session_cpp::Point> > &polylines, const bool &simple_case, const bool &remove_duplicates)
{
    std::string file_path = simple_case ? path_and_file_for_input_polylines_simple_case : path_and_file_for_input_polylines;
    std::string property_to_read = "input_polylines";

    printf ("read_wood::xml ->  read_xml_polylines -> ");
    printf ("%s", file_path.c_str ());
    printf ("\n");
    if (!file_exists_0 (file_path))
        {
            printf ("read_wood::xml -> read_wood::xml|read_xml_polylines|File "
                    "does not exist \n");
            return false;
        }
    else
        {
            printf ("read_wood::xml -> read_xml_polylines|file exists \n");
        }

    try
        {
            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_file (file_path.c_str ());
            if (!result)
                {
                    printf ("nread_wood::xml -> |read_xml_polylines|CPP Wrong "
                            "property \n");
                    return false;
                }

            pugi::xml_node root = doc.child (property_to_read.c_str ());
            for (pugi::xml_node poly_node : root.children ("Polyline"))
                {
                    std::vector<session_cpp::Point> polyline;
                    for (pugi::xml_node point_node : poly_node.children ("point"))
                        {
                            double x = point_node.child ("x").text ().as_double ();
                            double y = point_node.child ("y").text ().as_double ();
                            double z = point_node.child ("z").text ().as_double ();
                            session_cpp::Point p (x, y, z);

                            if (remove_duplicates)
                                if (polyline.size () > 0)
                                    if (session_cpp::Point::squared_distance (polyline.back (), p) < wood::GLOBALS::DISTANCE_SQUARED)
                                        continue;

                            polyline.emplace_back (p);
                        }
                    polylines.emplace_back (polyline);
                }
        }
    catch (std::exception &e)
        {
            (void)e;
            printf ("nread_wood::xml -> |read_xml_polylines|CPP Wrong "
                    "property \n");
            return false;
        }
    return true;
}

bool
read_xml_polylines (std::vector<std::vector<double> > &polylines, const bool &simple_case, const bool &remove_duplicates)
{
    std::string file_path = simple_case ? path_and_file_for_input_polylines_simple_case : path_and_file_for_input_polylines;
    std::string property_to_read = "input_polylines";

    printf ("read_wood::xml ->  read_xml_polylines -> ");
    printf ("%s", file_path.c_str ());
    printf ("\n");
    if (!file_exists_0 (file_path))
        {
            printf ("read_wood::xml -> read_wood::xml|read_xml_polylines|File "
                    "does not exist \n");
            return false;
        }
    else
        {
            printf ("read_wood::xml -> read_xml_polylines|file exists \n");
        }

    try
        {
            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_file (file_path.c_str ());
            if (!result)
                {
                    printf ("nread_wood::xml -> |read_xml_polylines|CPP Wrong "
                            "property \n");
                    return false;
                }

            pugi::xml_node root = doc.child (property_to_read.c_str ());
            for (pugi::xml_node poly_node : root.children ("Polyline"))
                {
                    std::vector<double> polyline;
                    for (pugi::xml_node point_node : poly_node.children ("point"))
                        {
                            double x = point_node.child ("x").text ().as_double ();
                            double y = point_node.child ("y").text ().as_double ();
                            double z = point_node.child ("z").text ().as_double ();
                            polyline.emplace_back (x);
                            polyline.emplace_back (y);
                            polyline.emplace_back (z);
                        }
                    polylines.emplace_back (polyline);
                }
        }
    catch (std::exception &e)
        {
            (void)e;
            printf ("nread_wood::xml -> |read_xml_polylines|CPP Wrong "
                    "property \n");
            return false;
        }
    return true;
}

bool
read_xml_polylines_and_properties (std::vector<std::vector<session_cpp::Point> > &input_polyline_pairs, std::vector<std::vector<session_cpp::Vector> > &input_insertion_vectors, std::vector<std::vector<int> > &input_JOINTS_TYPES,
                                   std::vector<std::vector<int> > &input_three_valence_element_indices_and_instruction, std::vector<int> &input_adjacency, const bool &simple_case, const bool &remove_duplicates)
{
    std::string file_path = simple_case ? path_and_file_for_input_polylines_simple_case : path_and_file_for_input_polylines;
    std::string property_to_read = "input_polylines";

    printf ("read_wood::xml ->  read_xml_polylines -> ");
    printf ("%s", file_path.c_str ());
    printf ("\n");
    if (!file_exists_0 (file_path))
        {
            printf ("read_wood::xml -> read_wood::xml|read_xml_polylines|File "
                    "does not exist \n");
            return false;
        }
    else
        {
            printf ("read_wood::xml -> read_xml_polylines|file exists \n");
        }

    try
        {
            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_file (file_path.c_str ());
            if (!result)
                {
                    printf ("nread_wood::xml -> |read_xml_polylines|CPP Wrong "
                            "property \n");
                    return false;
                }

            pugi::xml_node root = doc.child (property_to_read.c_str ());
            for (pugi::xml_node node : root.children ())
                {
                    std::string tag = node.name ();

                    if (tag == "polyline")
                        {
                            std::vector<session_cpp::Point> polyline;
                            for (pugi::xml_node point_node : node.children ("point"))
                                {
                                    double x = point_node.child ("x").text ().as_double ();
                                    double y = point_node.child ("y").text ().as_double ();
                                    double z = point_node.child ("z").text ().as_double ();
                                    session_cpp::Point p (x, y, z);

                                    if (remove_duplicates && polyline.size () > 0)
                                        if (session_cpp::Point::squared_distance (polyline.back (), p) < wood::GLOBALS::DISTANCE_SQUARED)
                                            continue;

                                    polyline.emplace_back (x, y, z);
                                }
                            input_polyline_pairs.emplace_back (polyline);
                        }
                    else if (tag == "insertion_vectors")
                        {
                            std::vector<session_cpp::Vector> vectors;
                            for (pugi::xml_node vec_node : node.children ("vector"))
                                {
                                    double x = vec_node.child ("x").text ().as_double ();
                                    double y = vec_node.child ("y").text ().as_double ();
                                    double z = vec_node.child ("z").text ().as_double ();
                                    vectors.emplace_back (x, y, z);
                                }
                            input_insertion_vectors.emplace_back (vectors);
                        }
                    else if (tag == "joints_types")
                        {
                            std::vector<int> joint_types;
                            for (pugi::xml_node id_node : node.children ())
                                {
                                    joint_types.emplace_back (id_node.text ().as_int ());
                                }
                            input_JOINTS_TYPES.emplace_back (joint_types);
                        }
                    else if (tag == "three_valence")
                        {
                            std::vector<int> three_valence;
                            for (pugi::xml_node id_node : node.children ())
                                {
                                    three_valence.emplace_back (id_node.text ().as_int ());
                                }
                            input_three_valence_element_indices_and_instruction.emplace_back (three_valence);
                        }
                    else if (tag == "adjacency")
                        {
                            for (pugi::xml_node id_node : node.children ())
                                {
                                    input_adjacency.emplace_back (id_node.text ().as_int ());
                                }
                        }
                }
        }
    catch (std::exception &e)
        {
            (void)e;
            printf ("nread_wood::xml -> |read_xml_polylines|CPP Wrong "
                    "property \n");
            return false;
        }

    if (input_insertion_vectors.size () > 0)
        if (input_insertion_vectors.size () != input_polyline_pairs.size () * 0.5)
            {
                std::cout << "\n" << input_polyline_pairs.size () * 0.5 << " " << input_insertion_vectors.size () << "\n";
                printf ("nread_wood::xml -> |read_xml_polylines|CPP "
                        "insertion vectors are given, but count "
                        "of them is not equal to polyline count \n");
                return false;
            }
    if (input_JOINTS_TYPES.size () > 0)
        if (input_JOINTS_TYPES.size () != input_polyline_pairs.size () * 0.5)
            {
                printf ("nread_wood::xml -> |read_xml_polylines|CPP joint "
                        "types are given, but count of "
                        "them is not equal to polyline count \n");
                std::cout << "\n" << input_polyline_pairs.size () << " " << input_JOINTS_TYPES.size () << "\n";
                return false;
            }
    return true;
}

bool
read_xml_polylines_and_properties (std::vector<std::vector<double> > &input_polyline_pairs, std::vector<std::vector<double> > &input_insertion_vectors, std::vector<std::vector<int> > &input_JOINTS_TYPES,
                                   std::vector<std::vector<int> > &input_three_valence_element_indices_and_instruction, std::vector<int> &input_adjacency, const bool &simple_case, const bool &remove_duplicates)
{
    std::string file_path = simple_case ? path_and_file_for_input_polylines_simple_case : path_and_file_for_input_polylines;
    std::string property_to_read = "input_polylines";

    printf ("read_wood::xml ->  read_xml_polylines -> ");
    printf ("%s", file_path.c_str ());
    printf ("\n");
    if (!file_exists_0 (file_path))
        {
            printf ("read_wood::xml -> wood::xml|read_xml_polylines | "
                    "File does not exist \n");
            return false;
        }
    else
        {
            printf ("read_wood::xml -> read_xml_polylines | file exists \n");
        }

    printf ("start reading \n");
    try
        {
            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_file (file_path.c_str ());
            if (!result)
                {
                    printf ("nread_wood::xml -> | read_xml_polylines | CPP Wrong "
                            "property \n");
                    return false;
                }

            pugi::xml_node root = doc.child (property_to_read.c_str ());
            for (pugi::xml_node node : root.children ())
                {
                    std::string tag = node.name ();

                    if (tag == "polyline" || tag == "Polyline")
                        {
                            std::vector<double> polyline;
                            for (pugi::xml_node point_node : node.children ("point"))
                                {
                                    double x = point_node.child ("x").text ().as_double ();
                                    double y = point_node.child ("y").text ().as_double ();
                                    double z = point_node.child ("z").text ().as_double ();
                                    polyline.emplace_back (x);
                                    polyline.emplace_back (y);
                                    polyline.emplace_back (z);
                                }
                            input_polyline_pairs.emplace_back (polyline);
                        }
                    else if (tag == "insertion_vectors")
                        {
                            std::vector<double> vectors;
                            for (pugi::xml_node vec_node : node.children ("vector"))
                                {
                                    double x = vec_node.child ("x").text ().as_double ();
                                    double y = vec_node.child ("y").text ().as_double ();
                                    double z = vec_node.child ("z").text ().as_double ();
                                    vectors.emplace_back (x);
                                    vectors.emplace_back (y);
                                    vectors.emplace_back (z);
                                }
                            input_insertion_vectors.emplace_back (vectors);
                        }
                    else if (tag == "joints_types")
                        {
                            std::vector<int> joint_types;
                            for (pugi::xml_node id_node : node.children ())
                                {
                                    joint_types.emplace_back (id_node.text ().as_int ());
                                }
                            input_JOINTS_TYPES.emplace_back (joint_types);
                        }
                    else if (tag == "three_valence")
                        {
                            std::vector<int> three_valence;
                            for (pugi::xml_node id_node : node.children ())
                                {
                                    three_valence.emplace_back (id_node.text ().as_int ());
                                }
                            input_three_valence_element_indices_and_instruction.emplace_back (three_valence);
                        }
                    else if (tag == "adjacency")
                        {
                            for (pugi::xml_node id_node : node.children ())
                                {
                                    input_adjacency.emplace_back (id_node.text ().as_int ());
                                }
                        }
                }
        }
    catch (std::exception &e)
        {
            (void)e;
            printf ("nread_wood::xml -> | read_xml_polylines | CPP Wrong "
                    "property \n");
            return false;
        }

    if (input_insertion_vectors.size () > 0)
        if (input_insertion_vectors.size () != input_polyline_pairs.size () * 0.5)
            {
                std::cout << "\n" << input_polyline_pairs.size () * 0.5 << " " << input_insertion_vectors.size () << "\n";
                printf ("nread_wood::xml -> |read_xml_polylines|CPP "
                        "insertion vectors are given, but count "
                        "of them is not equal to polyline count \n");
                return false;
            }
    if (input_JOINTS_TYPES.size () > 0)
        if (input_JOINTS_TYPES.size () != input_polyline_pairs.size () * 0.5)
            {
                printf ("nread_wood::xml -> |read_xml_polylines|CPP joint "
                        "types are given, but count of "
                        "them is not equal to polyline count \n");
                std::cout << "\n" << input_polyline_pairs.size () << " " << input_JOINTS_TYPES.size () << "\n";
                return false;
            }
    return true;
}

bool
write_xml_polylines (std::vector<std::vector<session_cpp::Point> > &polylines, const bool &simple_case)
{
    std::string file_path = simple_case ? path_and_file_for_output_polylines_simple_case : path_and_file_for_output_polylines;
    std::string property_to_write = "output_polylines";

    try
        {
            pugi::xml_document doc;
            pugi::xml_node root = doc.append_child (property_to_write.c_str ());

            for (auto &polyline : polylines)
                {
                    pugi::xml_node poly_node = root.append_child ("Polyline");
                    for (auto &point : polyline)
                        {
                            pugi::xml_node point_node = poly_node.append_child ("point");
                            point_node.append_child ("x").text ().set (point[0]);
                            point_node.append_child ("y").text ().set (point[1]);
                            point_node.append_child ("z").text ().set (point[2]);
                        }
                }

            if (!doc.save_file (file_path.c_str ()))
                {
                    printf ("write_wood::xml -> CPP Something went wrong, probaby "
                            "wrong path \n");
                    return false;
                }
        }
    catch (std::exception &e)
        {
            (void)e;
            printf ("write_wood::xml -> CPP Something went wrong, probaby "
                    "wrong path \n");
            return false;
        }
    return true;
}

bool
write_xml_polylines (std::vector<std::vector<std::vector<session_cpp::Point> > > &polylines_tree, int id)
{
    std::string file_path = path_and_file_for_output_polylines;
    std::string property_to_write = "output_polylines";

    try
        {
            pugi::xml_document doc;
            pugi::xml_node root = doc.append_child (property_to_write.c_str ());

            int count = 0;
            for (auto &polylines : polylines_tree)
                {
                    if (id != -1)
                        if (count != id)
                            {
                                count++;
                                continue;
                            }

                    pugi::xml_node group_node = root.append_child ("Polyline_Group");
                    for (auto &polyline : polylines)
                        {
                            pugi::xml_node poly_node = group_node.append_child ("Polyline");
                            for (auto &point : polyline)
                                {
                                    pugi::xml_node point_node = poly_node.append_child ("point");
                                    point_node.append_child ("x").text ().set (point[0]);
                                    point_node.append_child ("y").text ().set (point[1]);
                                    point_node.append_child ("z").text ().set (point[2]);
                                }
                        }
                    count++;
                }

            if (!doc.save_file (file_path.c_str ()))
                {
                    printf ("write_wood::xml -> CPP Something went wrong, probaby "
                            "wrong path \n");
                    return false;
                }
        }
    catch (std::exception &e)
        {
            (void)e;
            printf ("write_wood::xml -> CPP Something went wrong, probaby "
                    "wrong path \n");
            return false;
        }
    return true;
}

bool
write_xml_polylines_and_types (std::vector<std::vector<Polyline> > &polylines_tree, std::vector<std::vector<wood::cut::cut_type> > &types_tree, int id, bool simple_case)
{
    std::string property_to_write = "output_polylines";
    std::string file_path = simple_case ? path_and_file_for_output_polylines_simple_case : path_and_file_for_output_polylines;

    try
        {
            pugi::xml_document doc;
            pugi::xml_node root = doc.append_child (property_to_write.c_str ());

            int count = 0;
            for (auto &polylines : polylines_tree)
                {
                    if (id != -1)
                        if (count != id)
                            {
                                count++;
                                continue;
                            }

                    pugi::xml_node group_node = root.append_child ("polyline_group");
                    for (auto &polyline : polylines)
                        {
                            pugi::xml_node poly_node = group_node.append_child ("polyline");
                            for (auto &point : polyline)
                                {
                                    pugi::xml_node point_node = poly_node.append_child ("point");
                                    point_node.append_child ("x").text ().set (point[0]);
                                    point_node.append_child ("y").text ().set (point[1]);
                                    point_node.append_child ("z").text ().set (point[2]);
                                }
                        }
                    count++;
                }

            count = 0;
            for (auto &types : types_tree)
                {
                    if (id != -1)
                        if (count != id)
                            {
                                count++;
                                continue;
                            }

                    pugi::xml_node type_group = root.append_child ("type_group");
                    for (wood::cut::cut_type &type : types)
                        {
                            type_group.append_child ("type").text ().set (wood::cut::cut_type_to_string[type].c_str ());
                        }
                    count++;
                }

            if (!doc.save_file (file_path.c_str ()))
                {
                    printf ("write_wood::xml -> CPP Something went wrong, probaby "
                            "wrong path \n");
                    return false;
                }
        }
    catch (std::exception &e)
        {
            (void)e;
            printf ("write_wood::xml -> CPP Something went wrong, probaby "
                    "wrong path \n");
            return false;
        }
    return true;
}
} // namespace xml
} // namespace wood
