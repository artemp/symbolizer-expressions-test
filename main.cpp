#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <boost/variant.hpp>
#include <boost/timer/timer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/any.hpp>
#include <boost/spirit/include/support_utree.hpp>
// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/expression.hpp>
#include <mapnik/expression_evaluator.hpp>

#define BOOST_REGEX_HAS_ICU

namespace test {

typedef boost::variant<int, double, std::string> value_type;

struct symbolizer
{
    typedef std::map<std::string,value_type> cont_type;
    cont_type properties;
};

struct symbolizer_2
{
    double opacity;
    int num;
    std::string name;
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


template <typename T1>
struct extract_value : public boost::static_visitor<T1>
{
    typedef T1 return_type;

    template <typename T2>
    return_type operator() (T2 const& val) const
    {
        return boost::lexical_cast<return_type>(val);
    }

    return_type const& operator() (return_type const& val) const
    {
        return val;
    }
};

template <typename T>
void put(symbolizer & sym, std::string const& key, T const& val)
{
    sym.properties.insert(std::make_pair(key, val));
}

template <typename T>
T get(symbolizer const& sym, std::string const& key, T const& default_value = T())
{
    typedef symbolizer::cont_type::const_iterator const_iterator;
    const_iterator itr = sym.properties.find(key);
    if (itr != sym.properties.end())
    {
        return boost::apply_visitor(extract_value<T>(), itr->second);
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

    {
        boost::timer::auto_cpu_timer t;
        test::symbolizer_2 sym;
        sym.opacity = 0.5;
        sym.num = 123;
        sym.name = "test";
        for (int i=0; i< NUM_RUNS;++i)
        {
            double opacity = sym.opacity;
            int num = sym.num;
            std::string const& name = sym.name;
            if (i == 0)
            {
                std::cerr << opacity << std::endl;
                std::cerr << num << std::endl;
                std::cerr << name  << std::endl;
            }
        }
    }
    //
    {
        boost::timer::auto_cpu_timer t;
        test::symbolizer sym;
        test::put<double>(sym, "opacity", 0.5);
        test::put<int>(sym, "num", 123);
        test::put<std::string>(sym, "name", "testing");
        for (int i=0; i< NUM_RUNS;++i)
        {
            double opacity = test::get<double>(sym, "opacity");
            int num = test::get<int>(sym, "num");
            std::string name= test::get<std::string>(sym, "name");
            if (i == 0)
            {
                std::cerr << opacity << std::endl;
                std::cerr << num << std::endl;
                std::cerr << name  << std::endl;
            }
        }
    }

    {
        mapnik::feature_ptr feature(mapnik::feature_factory::create(std::make_shared<mapnik::context_type>(),1));
        boost::timer::auto_cpu_timer t;
        test::symbolizer_4 sym;
        test::put<double>(sym, "opacity", 0.5);
        test::put<int>(sym, "num", 123);
        test::put<mapnik::value_unicode_string>(sym, "name", "testing");
        for (int i=0; i< NUM_RUNS;++i)
        {
            double opacity = test::get<double>(sym, "opacity", *feature);
            int num = test::get<int>(sym, "num", *feature);
            std::string name = test::get<std::string>(sym, "name", *feature);
            if (i == 0)
            {
                std::cerr << opacity << std::endl;
                std::cerr << num << std::endl;
                std::cerr << name  << std::endl;
            }
            //std::string name= test::get<std::string>(sym, "name", *feature);
        }
    }

    std::cerr << "Done" << std::endl;
    return 0;
}
