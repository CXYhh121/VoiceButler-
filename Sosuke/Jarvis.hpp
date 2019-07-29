#ifndef __JARVIS_HPP__
#define __JARVIS_HPP__
#include <iostream>
#include <pthread.h>
#include <json/json.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <memory>
#include <map>
#include <unordered_map>
#include <cstdio>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <stdlib.h>
#include "speech.h"
#include "base/http.h"//直接使用百度提供的httpclient

#define ASR_PATH "temp_file/asr.wav"
#define CMD_PATH "command.etc"
#define PLAY_PATH "temp_file/play.mp3"
#define DEBUG "debug.txt"

//工具类
class Util{
    private:
        static pthread_t id;
    public:
        static bool Exec(std::string command,bool is_print)
        {
            if(!is_print){
                command += ">/dev/null 2>&1";
            }
            FILE *fp = popen(command.c_str(),"r");
            if(nullptr == fp){
                std::cerr << "popen exec \'" << command << "\'Error" <<std::endl;
                return false;
            }

            if(is_print){
                char ch;
                while(fread(&ch,1,1,fp) > 0){
                    fwrite(&ch,1,1,stdout);
                }
            }
            pclose(fp);
            return true;
        }
        static void *ThreadRun(void * arg)
        {
            pthread_detach(pthread_self());
            const char *tips = (char*)arg;
            int i = 0;
            char bar[53] = {0};
            const char* lable = "|/-\\";
            for(;i <= 50;i++){
                printf("%s[%-51s][%d%%][%c]\r",tips,bar,i*2,lable[i%4]);
                fflush(stdout);
                bar[i] = '=';
                bar[i+1] = '>';
                usleep(49000*2);
            }
            printf("\n");
        }
        static void PrintStart(std::string tips)
        {
            pthread_create(&id,NULL,ThreadRun,(void*)tips.c_str());
        }
        static void PrintEnd()
        {
            pthread_cancel(id);
        }
};
pthread_t Util::id;

//机器人类
class Robot{
    private:
        std::string url;
        std::string api_key;
        std::string user_id;
        aip::HttpClient client;//定义一个百度提供的http客户端，以调用其相关接口

    private:

         bool IsCodeLegal(int code)
         {
             bool result = true;
             switch(code){
                 case 5000:
                     result = false;
                     break;
                 case 6000:
                     result = false;
                     break;
                 case 4000:
                     result = false;
                     break;
                 default: 
                     break;
             }
             return result;
         }
        //将我们的信息转换为json串
        std::string MessageToJson(std::string& message)
        {
            Json::Value root;//可以看做是一个万能对象，什么类型的信息都可以存放在这个对象中，方便序列化
            Json::StreamWriterBuilder wb;
            std::ostringstream ss;

            Json::Value item_;
            item_["text"] = message;

            Json::Value item;
            item["inputText"] = item_;

            //输入类型:0-文本(默认)、1-图片、2-音频
            root["reqType"] = 0;  //text
            root["perception"] = item;
            
            item.clear();
            item["apiKey"] = api_key;
            item["userId"] = user_id;
            root["userInfo"] = item; 

            //将new好的资源交给智能指针管理，以免忘记释放导致内存泄漏
            std::unique_ptr<Json::StreamWriter> sw(wb.newStreamWriter());
            //创建好的对象调用Write接口将root中的信息写入ss中
            sw->write(root,&ss);
            std::string json_string =  ss.str();
            //std::cout << "debug: " << json_string << std::endl;
            return json_string;
        }
        
        //向图灵机器人，发起http post请求
        //这里要使用开源jsoncpp进行序列化和反序列化
        std::string PostRequest(std::string &request_json)
        {
            //http Post请求返回的结果 
            std::string response;
            //用我们创建的httpclient对象调用百度内部的post请求接口
            //其实我们也可以自己写post请求，使用socket套接字，但是为了简便我们直接使用百度提供的post接口
            int status_code = client.post(url,nullptr,request_json,nullptr,&response);
            if(status_code != CURLcode::CURLE_OK){//post请求失败，返回空
                std::cerr << "http post request error!" << std::endl;
                return "";
            }
            //std::cout << "debug:"<< response << std::endl;
            return response;
        }
        
        //将图灵机器人返回的json串转换为文本信息
        std::string JsonToEchoMessage(std::string& str)
        {
            //std::cout << "JsonToEchoMessage: " << str << std::endl;
            JSONCPP_STRING errs;
            Json::Value root;
            Json::CharReaderBuilder rb;

            //将new好的资源交给智能指针管理，以免忘记释放导致内存泄漏
            std::unique_ptr<Json::CharReader> const cr(rb.newCharReader());
            
            //用创建好的cr对象调用parse接口解析response中的信息
            bool res = cr->parse(str.data(),str.data()+str.size(),&root,&errs);
            if(!res || !errs.empty()){//如果解析为空或是有错误
                std::cerr << "http post parse error" << errs << std::endl;
                return "";
            }

            int code = root["intent"]["code"].asInt();
            if(!IsCodeLegal(code)){//返回码若是错误
                std::cerr << "response code error" << std::endl;
                return "";
            }

            Json::Value item = root["results"][0];
            std::string msg = item["values"]["text"].asString();
            return msg;
        }
    public:
        Robot(std::string id = "1")
            :user_id(id)
        {
            this->url = "http://openapi.tuling123.com/openapi/api/v2";
            this->api_key = "ce3aa04c19864c7e9896df457095a1a1";
        }

        std::string Talk(std::string message)
        {
            //将我们输入的文本信息转换为http请求参数格式json
            std::string request_json = MessageToJson(message);
            //发起http请求，连接图灵机器人
            std::string response_json = PostRequest(request_json);
            //将http响应回来的信息转换为文本信息
            std::string echo_message = JsonToEchoMessage(response_json);
            //std::cout << echo_message << std::endl;
            return echo_message;
        }
        ~Robot()
        {}
};


//语音识别类
class SpeechRec{
    private:
        std::string app_id;
        std::string api_key;
        std::string secret_key;       
        aip::Speech *client;
    private: 
        bool IsCodeLegal(int code, std::string& err_msg)
        {
            bool result = false;
            switch(code){
                case 0:
                    result = true;
                    break;
                case 3300:
                    err_msg = "用户输入错误";
                    break;
                defalut:
                    std::cerr << "code error..." << std::endl;
                    break;
            }
            return result;
        }
    public:
        SpeechRec()
        {
            app_id = "16896727";
            api_key = "k5ki5A4rxfVSt08v5L4Uy0Aw";
            secret_key = "q9LaKxY35ssV4i6sDMa6aNXwEY0sUTU0";       
            client = new aip::Speech(app_id,api_key,secret_key);
        }
    
        //语音识别接口
        bool Speech2Text(std::string path,std::string& out)
        {
            std::map<std::string,std::string> options;
            options["dev_pid"] = "1536";
            std::string file_content;
            aip::get_file_content(ASR_PATH,&file_content);

            //Speech创建的客户端对象调用语音识别接口recognize识别语音信息
            Json::Value result = client->recognize(file_content,"wav",16000,options);
            
            //std::cout << "debug: " << result.toStyledString() << std::endl;
            
            int code = result["err_no"].asInt();
            std::string err_msg;
            if(!IsCodeLegal(code,err_msg)){
                std::cout << "Recognize error:" << err_msg << std::endl;
                return false;
            }
            
            //获取返回值中的result信息，是一个数组
            out = result["result"][0].asString();
            return true;
        }

        //将图灵机器人返回的消息转换为语音消息
        bool Text2Speech(std::string message)
        {
            bool ret;
            std::ofstream ofile;
            std::string ret_file;
            std::map<std::string, std::string> options;
            options["spd"] = "6";//语速0~15
            options["pit"] = "5";//音调0~15
            options["vol"] = "15";//音量0~15
            options["per"] = "1";//语音类别1,0,3,4,106,110,111
            ofile.open(PLAY_PATH, std::ios::out | std::ios::binary);

            //语音合成，将文本转成语音，放到指定目录，形成指定文件
            Json::Value result = client->text2audio(message, options, ret_file);
            if(!ret_file.empty()){
                ofile << ret_file;
                ret = true;
            }
            else{
                std::cout << "error: " << result.toStyledString() << std::endl;
                ret = false;
            }
            ofile.close();
            return ret;
        }
        ~SpeechRec()
        {}
};


//核心逻辑实现类
class Jarvis{
    private:
        Robot rt;
        SpeechRec sr;
        std::unordered_map<std::string,std::string> commands;
    private:
        //录音接口
        bool Record()
        {
            std::cout << "debug:Record....." << std::endl;
            std::string command = "arecord -t wav -c 1 -r 16000 -d 5 -f S16_LE ";
            command += ASR_PATH;
            bool ret = Util::Exec(command,false);
            std::cout <<"debug:Record done...." << std::endl;
            return ret;
        }

        //播放机器人语音
        bool Aplay()
        {
            std::string command = "cvlc --play-and-exit "; //Exit after playing.
            command += PLAY_PATH;
            if(!Util::Exec(command, false))
            {
                std::cerr<< " Play audio error." << std::endl;
                return false;
            }
            return true;
        }

        //判断message是否是命令，是则返回该命令的执行语句
        //eg：查看当前目录:ls -l
        bool IsCommand(std::string& message, std::string& cmd)
        {
            auto iter = commands.find(message);
            if(iter == commands.end())
                return false;
            else{
                cmd = iter->second;
                return true;
            }
        }
    public:
        Jarvis()
        {}
        
        //加载命令配置文件
        bool LoadEtc()
        {
            std::ifstream in(CMD_PATH);
            if(!in.is_open()){
                std::cerr << "open error" << std::endl;
                return false;
            }
            char line[256];
            std::string sep = ":";
            while(in.getline(line,sizeof(line))){
                std::string str = line;
                std::size_t pos = str.find(sep);
                if(std::string::npos == pos){
                    std::cerr << "not find : " << std::endl;
                    continue;
                }
                std::string key = str.substr(0,pos);
                std::string value = str.substr(pos+sep.size());
                key += "。";
                //commands.insert(std::make_pair(key,value));
                commands.insert({key,value});
            }
            std::cerr << "Load command etc done.... success" << std::endl;

            in.close();
            return true;    
        }

        //核心逻辑执行
        void Run()
        {
#ifdef _DEBUG_
            int fd = open("DEBUG",O_WRONLY|O_CREAT,0644);
            dup2(fd,2);
#endif
            volatile bool quit = false;
            while(!quit){
                std::string tips = "Record....";
                Util::PrintStart(tips);
                if(this->Record()){
                    std::string message;
                    if(sr.Speech2Text(ASR_PATH,message)){
                        std::string cmd = "";
                        //1.判断是否是命令，是则执行
                        if(IsCommand(message,cmd)){
                            std::cout << "[Sosuke@lcalhost]$ " << cmd << std::endl;
                            Util::Exec(cmd,true);
                            continue;
                        }else{
                            // std::cout << "我# " << message <<std::endl;
                            if(message == "你走吧。"){
                                std::cout << "我# " << message <<std::endl;
                                std::string quit_message =  "我走了，不要想我哦";
                                std::cout << "Sosuke# " << quit_message << std::endl;
                                if(sr.Text2Speech(quit_message)){   //如果语音合成成功
                                    this->Aplay();
                                }
                                exit(0);
                            }
                            //2.识别不是命令，交给图灵机器人
                            std::cout << "我# " << message << std::endl;
                            std::string echo = rt.Talk(message);
                            std::cout << "Sosuke# " << echo << std::endl;
                            if(sr.Text2Speech(echo)){   //如果语音合成成功
                                //调用播放音频函数播放机器人消息
                                this->Aplay();
                            }
                        }
                    }else{
                        std::cerr << "Recognize error.." << std::endl;
                    }
                }
                else{
                    std::cerr << "Record error...." << std::endl;
                }
                Util::PrintEnd();
            }
#ifndef _DEBUG_
            close(fd);
#endif
        }
        ~Jarvis();

};

#endif
