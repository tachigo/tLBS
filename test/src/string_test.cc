//
// Created by 刘立悟 on 2020/6/1.
//

#include <iostream>

using namespace std;

int main() {
    string s = "\r\n\t \t这是随便写的一句话。\t\t ";
    size_t n = s.find_last_not_of( " \r\n\t" );
    if( n != string::npos )
    {
            s.erase( n + 1 , s.size() - n );
    }

    n = s.find_first_not_of ( " \r\n\t" );
    if( n != string::npos )
    {
            s.erase( 0 , n );
    }
}