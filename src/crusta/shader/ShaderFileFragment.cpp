#include <crusta/shader/ShaderFileFragment.h>


#include <sys/stat.h>

#include <crusta/vrui.h>

namespace crusta {


ShaderFileFragment::
ShaderFileFragment(const std::string& shaderFileName_) :
    fileName(shaderFileName_), fileStamp(0)
{
}

bool ShaderFileFragment::
checkFileForChanges() const
{
    struct stat statRes;
    if (stat(fileName.c_str(), &statRes) == 0)
    {
        if (statRes.st_mtime != fileStamp)
            return true;
        else
            return false;
    }
    else
    {
        Misc::throwStdErr("ShaderFileFragment: can't check source file (%s)",
                          fileName.c_str());
        return false;
    }
}

void ShaderFileFragment::
extractToken(std::ifstream& stream, std::string& token, TokenArgs& args)
{
    //clear the token and the arguments
    token.clear();
    args.clear();

    char c;
    std::string* dst = &token;
    while (stream.good())
    {
        //grab the next characther from the stream
        c = stream.get();
        //check if we are done
        if (c=='#' && stream.peek()=='#')
        {
            //discard the second #
            c = stream.get();
            //return the extracted elements
            return;
        }
        //check if we are switching to an argument
        else if (c=='#')
        {
            //create a new argument and switch the destination to it
            args.push_back(std::string());
            dst = &args.back();
        }
        //add the character (if not EOF) to the current destination (token/arg)
        else if (c!=EOF)
        {
            *dst += c;
        }
    }
}

std::string ShaderFileFragment::
readCodeFromFile()
{
    char c;
    std::string code;
    std::string token;
    TokenArgs   args;
    std::ifstream file(fileName.c_str());
    while (file.good())
    {
        //grab the next character from the stream
        c = file.get();
        //check if we are seeing the start of a token
        if (c=='#' && file.peek()=='#')
        {
            //discard the second #
            c = file.get();
            //extract the token and its arguments
            extractToken(file, token, args);
            //replace the token and append the proper code
            code += replaceToken(token, args);
        }
        //if we haven't encountered a token just pass along the (!EOF) character
        else if (c!=EOF)
        {
            code += c;
        }
    }

    return code;
}


bool ShaderFileFragment::
update()
{
    return checkFileForChanges();
}

std::string ShaderFileFragment::
getCode()
{
    if (codeEmitted)
        return "";

    std::string code = readCodeFromFile();

    //update the age of the loaded code
    struct stat statRes;
    stat(fileName.c_str(), &statRes);
    fileStamp = statRes.st_mtime;

    codeEmitted = true;
    return code;
}


} //namespace crusta
