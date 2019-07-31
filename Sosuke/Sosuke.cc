#include "Sosuke.hpp"

using namespace std;
int main()
{
   //Robot rt;
   //string str;
   //volatile bool quit = false;
   //while(!quit){
   //    cout << "我# ";
   //    cin >> str;
   //    string s = rt.Talk(str);
   //    cout << "Sosuke# " << s << endl;
   //}
   Sosuke* js = new Sosuke();
   while(true)
   {
       if(!js->LoadEtc()){
           return 1;
       }
       js->Run();
   }
   return 0;
}

