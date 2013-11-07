#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <boost/variant.hpp>
#include "timer.hpp"
#include <boost/any.hpp>
#include <boost/spirit/include/support_utree.hpp>
// mapnik
#include <mapnik/color.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/expression.hpp>
#include <mapnik/expression_evaluator.hpp>
#include <mapnik/path_expression.hpp>
#include <mapnik/parse_path.hpp>
#include <mapnik/symbolizer.hpp>

namespace test {

struct symbolizer_base
{
    typedef boost::variant<bool,
                           int,
                           double,
                           std::string,
                           mapnik::color,
                           mapnik::expression_ptr,
                           mapnik::path_expression_ptr,
                           mapnik::transform_type> value_type;
    typedef std::string key_type;
    typedef std::map<key_type, value_type> cont_type;
    cont_type properties;
};

struct point_symbolizer : symbolizer_base {};
struct line_symbolizer : symbolizer_base {};
struct polygon_symbolizer : symbolizer_base {};

struct symbolizer_2
{
    double opacity;
    int num;
    std::string name;
    mapnik::color fill;
};

struct symbolizer_3
{
    typedef std::vector<boost::spirit::utree> cont_type;
    cont_type properties;
};

struct symbolizer_4
{
    typedef std::map<std::string, mapnik::expression_ptr> cont_type;
    cont_type properties;
};


template <typename T>
struct evaluate_path_wrapper
{
    typedef T result_type;
    template <typename T1, typename T2>
    result_type operator() (T1 const& expr, T2 const& feature) const
    {
        return result_type();
    }

};

template <>
struct evaluate_path_wrapper<std::string>
{
    template <typename T1, typename T2>
    std::string operator() (T1 const& expr, T2 const& feature) const
    {
        return mapnik::path_processor_type::evaluate(expr, feature);
    }
};


template <typename T>
struct evaluate_expression_wrapper
{
    typedef T result_type;
    template <typename T1, typename T2>
    result_type operator() (T1 const& expr, T2 const& feature) const
    {
        mapnik::value_type result = boost::apply_visitor(mapnik::evaluate<mapnik::feature_impl,mapnik::value_type>(feature), expr);
        return result.convert<result_type>();
    }
};

// specializations for not supported types
// mapnik::color
template <>
struct evaluate_expression_wrapper<mapnik::color>
{
    template <typename T1, typename T2>
    mapnik::color operator() (T1 const& expr, T2 const& feature) const
    {
        return mapnik::color();
    }
};


template <typename T1>
struct extract_value : public boost::static_visitor<T1>
{
    typedef T1 result_type;

    extract_value(mapnik::feature_impl const& feature)
        : feature_(feature) {}

    auto operator() (mapnik::expression_ptr const& expr) const -> result_type
    {
        return evaluate_expression_wrapper<result_type>()(*expr,feature_);
    }

    auto operator() (mapnik::path_expression_ptr const& expr) const -> result_type
    {
        return evaluate_path_wrapper<result_type>()(*expr, feature_);
    }

    auto operator() (result_type const& val) const -> result_type
    {
        return val;
    }

    template <typename T2>
    auto operator() (T2 const& val) const -> result_type
    {
        //std::cerr << val << " " << typeid(val).name() <<" " << typeid(result_type).name() << std::endl;
        return result_type();
    }

    mapnik::feature_impl const& feature_;
};

template <typename T1>
struct extract_raw_value : public boost::static_visitor<T1>
{
    typedef T1 result_type;

    auto operator() (result_type const& val) const -> result_type const&
    {
        return val;
    }

    template <typename T2>
    auto operator() (T2 const& val) const -> result_type
    {
        //std::cerr << val << " " << typeid(val).name() <<" " << typeid(result_type).name() << std::endl;
        return result_type();
    }
};


template <typename T>
void put(symbolizer_base & sym, std::string const& key, T const& val)
{
    sym.properties.insert(std::make_pair(key, val));
}

template <typename T>
T get(symbolizer_base const& sym, std::string const& key, mapnik::feature_impl const& feature, T const& default_value = T())
{
    typedef symbolizer_base::cont_type::const_iterator const_iterator;
    const_iterator itr = sym.properties.find(key);
    if (itr != sym.properties.end())
    {
        return boost::apply_visitor(extract_value<T>(feature), itr->second);
    }
    return default_value;
}

template <typename T>
T get(symbolizer_base const& sym, std::string const& key, T const& default_value = T())
{
    typedef symbolizer_base::cont_type::const_iterator const_iterator;
    const_iterator itr = sym.properties.find(key);
    if (itr != sym.properties.end())
    {
        return boost::apply_visitor(extract_raw_value<T>(), itr->second);
    }
    return default_value;
}


// 4
template <typename T>
T get(symbolizer_4 const& sym, std::string const& key, mapnik::feature_impl const& feature, T const& default_value = T())
{
    typedef symbolizer_4::cont_type::const_iterator const_iterator;
    const_iterator itr = sym.properties.find(key);
    if (itr != sym.properties.end())
    {
        mapnik::expression_ptr const& expr = itr->second;
        if (expr)
        {
            mapnik::value_type result = boost::apply_visitor(mapnik::evaluate<mapnik::feature_impl,mapnik::value_type>(feature), *expr);
            return result.convert<T>();
        }
    }
    return default_value;
}

template <typename T>
void put(symbolizer_4 & sym, std::string const& key, T const& val)
{
    sym.properties.insert(std::make_pair(key, std::make_shared<mapnik::expr_node>(val)));
}

}


int main (int argc, char** argv)
{

    if (argc!=2)
    {
        std::cerr << "Usage:" << argv[0] << " <num-runs>" << std::endl;
        return 1;
    }

    const int NUM_RUNS=std::stol(argv[1]);

    std::cerr << "Running " << NUM_RUNS << " times.." << std::endl;

     //
    {
        progress_timer test1(std::clog,"test 1");
        mapnik::feature_ptr feature(mapnik::feature_factory::create(std::make_shared<mapnik::context_type>(),1));
        mapnik::transcoder tr("utf8");
        feature->put_new("name",tr.transcode("mapnik"));
        feature->put_new("val",123.4567);
        test::point_symbolizer sym;
        test::put<bool>(sym, "true", true);
        test::put<bool>(sym, "false", false);
        test::put(sym,"opacity", 0.5);
        test::put(sym,"num", 123);
        test::put(sym,"name", std::string("testing"));
        test::put(sym,"fill", mapnik::color("navy"));
        test::put(sym,"file-path", mapnik::parse_path("/Users/artem/Projects/[name]"));
        test::put(sym,"expr",mapnik::parse_expression("[val]*[val] + 877"));
        for (int i=0; i< NUM_RUNS; ++i)
        {
            bool true_ = test::get<bool>(sym,"true", *feature);
            bool false_ = test::get<bool>(sym,"false", *feature);
            double opacity = test::get<double>(sym,"opacity", *feature);
            mapnik::value_integer num = test::get<mapnik::value_integer>(sym,"num", *feature);
            std::string name= test::get<std::string>(sym,"name", *feature);
            mapnik::color fill = test::get<mapnik::color>(sym,"fill", *feature);
            std::string file_path = test::get<std::string>(sym,"file-path",*feature);
            mapnik::value_integer expr = test::get<mapnik::value_integer>(sym,"expr",*feature);
            mapnik::expression_ptr expr_ptr = test::get<mapnik::expression_ptr>(sym,"expr");

            if (i == 0)
            {
                std::cerr << true_ << std::endl;
                std::cerr << false_ << std::endl;
                std::cerr << opacity << std::endl;
                std::cerr << num << std::endl;
                std::cerr << name  << std::endl;
                std::cerr << fill << std::endl;
                std::cerr << file_path << std::endl;
                std::cerr << expr << std::endl;
                std::cerr << expr_ptr << std::endl;
            }
        }
    }

    {
        progress_timer test2(std::clog,"test 2");
        test::symbolizer_2 sym;
        sym.opacity = 0.5;
        sym.num = 123;
        sym.name = "testing";
        sym.fill = mapnik::color("navy");
        for (int i=0; i< NUM_RUNS;++i)
        {
            double opacity = sym.opacity;
            int num = sym.num;
            std::string const& name = sym.name;
            mapnik::color const& fill = sym.fill;
            if (i == 0)
            {
                std::cerr << opacity << std::endl;
                std::cerr << num << std::endl;
                std::cerr << name  << std::endl;
                std::cerr << fill << std::endl;
            }
        }
    }


    {
        progress_timer test3(std::clog,"test 3");
        mapnik::feature_ptr feature(mapnik::feature_factory::create(std::make_shared<mapnik::context_type>(),1));
        test::symbolizer_4 sym;
        test::put<double>(sym, "opacity", 0.5);
        test::put<mapnik::value_integer>(sym, "num", 123LL);
        test::put<mapnik::value_unicode_string>(sym, "name", "testing");
        for (int i=0; i< NUM_RUNS;++i)
        {
            double opacity = test::get<double>(sym, "opacity", *feature);
            mapnik::value_integer num = test::get<mapnik::value_integer>(sym, "num", *feature);
            std::string name = test::get<std::string>(sym, "name", *feature);
            if (i == 0)
            {
                std::cerr << opacity << std::endl;
                std::cerr << num << std::endl;
                std::cerr << name  << std::endl;
            }
        }
    }

    std::cerr << "Done" << std::endl;
    return 0;
}
