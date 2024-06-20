/**
 * @file
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 */

#ifndef _CGICPP_PARSER_BASE_HPP_
#define _CGICPP_PARSER_BASE_HPP_
#include <string>
using namespace std;

class CgiParserBase
{
  private:
    /* data */
    string parser_name;

  public:
    CgiParserBase(string name)
    {
        this->parser_name = name;
    };
    ~CgiParserBase();

    virtual void parse_local_for_each(void)
    {
        return;
    };
    virtual void parse_local_for_one(string opt)
    {
        return;
    };
    virtual void get_upload_json(string &str)
    {
        return;
    };
    virtual void set_remote_string(string input_string)
    {
        return;
    };
    virtual void set_local_for_each(void)
    {
        return;
    };
    virtual void set_local_for_one(string opt)
    {
        return;
    };

    string get_parser_name(void)
    {
        return this->parser_name;
    };
};

int main_parser_get(CgiParserBase *obj, string option, string input_json, string &rsp);
int main_parser_set(CgiParserBase *obj, string option, string input_json, string &rsp);

#endif