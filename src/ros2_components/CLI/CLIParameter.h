#ifndef CLIPARAMETER_H
#define CLIPARAMETER_H

#include <string>
#include <memory>
namespace ros2_components
{
/**
 * @brief The CLIParameter class represents a parameter of a verb. Forexample you provide a verb named build for you program you might want to add a parameter for a package or source file name.
 * This would lead to the following syntax:
 * <programname> build <filename>
 * The parameters for a verb are order sensitive. That means if you add multiple parameters they have to be supplied in the order you have added them.
 */
class CLIParameter
{
public:
    typedef std::shared_ptr<CLIParameter> SharedPtr;
    /**
     * @brief CLIParameter
     * @param _name
     * @param _description
     * @param _param
     */
    CLIParameter(std::string _name, std::string _description, std::string* _param);
    /**
     * @brief parse
     * @param parameter
     * @return true if successfully parsed. The parameter implemented currently always returns true
     * @exception May throw a std::invalid_argument exception if std::string* param was NULL.
     */
    bool parse(std::string parameter);
    /**
     * @brief getName
     * @return The name of the parameter.
     */
    std::string getName() const;

    /**
     * @brief getDescription
     * @return The description of the parameter
     */
    std::string getDescription() const;

private:
    std::string name;
    std::string description;
    std::string* param;

};
}

#endif // CLIPARAMETER_H
