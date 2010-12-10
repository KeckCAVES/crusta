#ifndef _Crusta_ShaderFileFragment_H_
#define _Crusta_ShaderFileFragment_H_


#include <ctime>
#include <fstream>
#include <string>

#include <crusta/shader/ShaderFragment.h>


BEGIN_CRUSTA


class ShaderFileFragment : public ShaderFragment
{
public:
    ShaderFileFragment(const std::string& shaderFileName_);

protected:
    typedef std::vector<std::string> TokenArgs;

    /** replace a token and it's arguments from the code read in from file with
        the proper string */
    virtual std::string replaceToken(const std::string& token,
                                     const TokenArgs& args) = 0;

    /** check for external changes to the file providing the shader code */
    bool checkFileForChanges() const;

    /** extract a token and its arguments from the input file stream */
    void extractToken(std::ifstream& stream,
                      std::string& token, TokenArgs& args);
    /** read in the shader code from the file */
    std::string readCodeFromFile();

    /** name of the file providing the shader code */
    std::string fileName;
    /** time stamp of the file content currently loaded (used to check for
        external changes to the file) */
    time_t fileStamp;

//- inherited from ShaderFragment
public:
    virtual bool update();
    virtual std::string getCode();
};


END_CRUSTA


#endif //_Crusta_ShaderFileFragment_H_
