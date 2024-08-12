#include "xml.h"
#include <stack>
#include <sstream>

namespace odb {

XML::XML() { }

XML::~XML() { }

void XML::parseXML(std::string filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    _name = filename.substr(filename.find_last_of('/') + 1);
    _value = "";

    std::stack<XML*> elementStack;
    std::string line;

    elementStack.push(this);


    while (getline(file, line)) {
        std::stringstream ss(line);
        std::string indent;
        std::string token;
        std::string value;

        getline(ss, indent, '<');
        getline(ss, token, '>');

        if (token.empty()) {
            continue;
        }

        if (token[0] == '/') {  // End tag
            if (!elementStack.empty()) {
                elementStack.pop();
            }
            continue;
        }
        else if (token[0] == '?') {  // XML declaration
            continue;
        }
        else if (token[token.length() - 1] == '/') {  // Self-closing tag
            XML newElement;
            newElement._name = token.substr(0, token.length() - 1);
            if (!elementStack.empty()) {
                elementStack.top()->_children.push_back(newElement);
            }
        }
        else{
            XML newElement;
            newElement._name = token;
            if (!elementStack.empty()) {
                elementStack.top()->_children.push_back(newElement);
            }
            elementStack.push(&elementStack.top()->_children.back());
        }

        if(getline(ss, value, '<') && getline(ss, token, '>')) {
            elementStack.top()->_value = value;
            elementStack.pop();
        }
    }

    if(elementStack.size() != 1) {
      // printf("Element stack size: %lu\n", elementStack.size());
      // while(!elementStack.empty()){
      //   printf("Element: %s\n", elementStack.top()->_name.c_str());
      //   elementStack.pop();
      // }
      throw std::runtime_error("Invalid XML file");
    }

    file.close();
}

std::string XML::to_string(int depth) const
{
    std::string indent(depth * 2, ' ');
    indent += _name + ": " + _value + "\n";
    for (const auto& child : _children) {
        indent += child.to_string(depth + 1);
    }
    return indent;
}

std::vector<XML>& XML::getChildren(){
    return _children;
}

std::string XML::getName(){
    return _name;
}

std::string XML::getValue(){
    return _value;
}

XML* XML::findChild(std::string name){
    for(auto& child : _children){
        if(child.getName() == name){
            return &child;
        }
    }
    return nullptr;
}

} // namespace odb

