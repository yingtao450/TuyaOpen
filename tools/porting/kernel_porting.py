#!/usr/bin/python3
# -*- coding:utf-8 -*-

from cgi import print_form
from curses import flash
import os
from pdb import pm
import sys
import shutil
import re
import os
import sys
import shutil
import re
import porting_new_file
import porting_old_file

file_desc  = "/**\n\
 * @file ###FILE###\n\
 * @brief this file was auto-generated by tuyaos v&v tools, developer can add implements between BEGIN and END\n\
 * \n\
 * @warning: changes between user 'BEGIN' and 'END' will be keeped when run tuyaos v&v tools\n\
 *           changes in other place will be overwrited and lost\n\
 *\n\
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.\n\
 * \n\
 */"  

# User-defined area start and end markers for additional headers, types, enums, macros, constants, global variables, internal functions, etc., as needed by developers
udf_flags_begin = "// --- BEGIN: user defines and implements ---\n"
udf_flags_end   = "// --- END: user defines and implements ---" 

# Function body start and end markers for additional developer-specific function implementations
func_body_flags_begin = "    // --- BEGIN: user implements ---"
func_body_flags_end   = "    // --- END: user implements ---"

DEFAULT_RETURN={"OPERATE_RET":"OPRT_NOT_SUPPORTED", "VOID*":"NULL", "VOID":"", "VOID_T*":"NULL", "VOID_T":"", "UINT64_T":"0", "INT64_T":"0", "UINT32_T":"0", "INT32_T":"0", "INT_T":"0", "UINT_T":"0", "UINT16_T":"0", "INT16_T":"0", "UINT8_T":"0", "INT8_T":"0", "CHAR_T":"0", "SCHAR_T":"0"}


class porting(object):
    def __init__(self, path, target, template=None) -> None:
        self.target = target
        self.target_path = path
        self.target_tuyaos_path = self.target_path + "/tuyaos/"
        self.target_tuyaos_adapter_path = self.target_tuyaos_path + "/tuyaos_adapter/"
        self.target_tuyaos_kernel_config_path = self.target_path + "/default.config"

        self.porting_path = self.target_path + "/../../tools/porting/"
        self.porting_template_path = self.porting_path + "template/"
        self.porting_template_linux_path = self.porting_template_path + "linux/"
        self.porting_template_bsp_path = self.porting_template_path + "bsp/"
        self.porting_template_rtos_path = self.porting_template_path + "rtos/"
        
        self.dirs = []
        self.template = template

        # print(self.target_path)
        # print(self.target_tuyaos_kernel_config_path)
        if (os.path.exists(self.target_tuyaos_kernel_config_path)):
            self.__get_ability()
            self.__load_files()
            print("find platform: ", self.target)
            print("     platform kernel config is ", self.target_tuyaos_kernel_config_path)
            print("     platform abilitys: ", self.abilitys)
            if self.abilitys['OPERATING_SYSTEM'] == '100':
                if self.template == 'bsp':
                    self.template_path = self.porting_template_bsp_path
                elif self.template == 'null':
                    self.template_path = None
                else:
                    self.template_path = self.porting_template_linux_path
            else:
                if self.template == 'null':
                    self.template_path = None
                else:
                    self.template_path = self.porting_template_rtos_path
        else:
            print("no found platform: ", self.target)
        pass

    def __get_ability(self):
        self.abilitys = {}
        config_file = open(self.target_tuyaos_kernel_config_path)
        config_content = config_file.read()
        kernel_template = re.findall("(.+=.+)", config_content)
        
        # Parse configuration
        for ability in kernel_template:
            c = re.split("=", ability)
            if c[1] == '""' : # Skip some special configurations
                continue
            
            # Special handling
            self.abilitys[c[0][7:]] = c[1] 

    def __merge_new_old_file(self, nfile, ofile):
        # Custom include and type
        nfile['udfs'] = ofile['udfs']

        # Same or new functions
        for func1 in nfile['funcs']:
            no_match_func = True
            for func2 in ofile['funcs']: 
                if  func1['name'] == func2['name']:
                    no_match_func = False
                    func1['body'] = func2['body'] # Only need to copy the body
            #print("new or merged func: ", func1)
            func1['isnew'] = no_match_func

        # Deleted functions (keep them)
        for func2 in ofile['funcs']:
            no_match_func = True
            for func1 in nfile['funcs']:
                if  func1['name'] == func2['name']:
                    no_match_func = False
            
            # Also keep the ones that have been deleted to avoid missing them
            if no_match_func:
                print("no matched func resloved: ", func2)
                func2['isnew'] = False
                func2['head'] = "/** this api was removed from kernel **/" + func2['head']
                nfile['funcs'].append(func2)

        return nfile

    def __load_files(self):
        # for each dir
        last_dir_name = ""
        for root, _, files in os.walk(self.target_tuyaos_adapter_path +  "include/"):
        
            if -1 != root.find("init"):
                continue

            if -1 != root.find("utilities"):
                continue

            print("----------------------------")
            print(root, files)
            print("----------------------------")

            dir_name = os.path.basename(root)
            if last_dir_name == "" or last_dir_name != dir_name: 
                dir = {}
                dir_files = []       
                dir['name'] = dir_name
                last_dir_name = dir_name

            for f in files:
                nfile = porting_new_file.parse_new_file(os.path.join(root, f)).load_file()         
                ofile = porting_old_file.parse_old_file(self.target_tuyaos_adapter_path + "src/" + re.sub("\.h", ".c", f)).load_file()
                if ofile:
                    # merge old and new file
                    mfile = self.__merge_new_old_file(nfile, ofile)
                    mfile['name'] = re.sub("\.h", ".c", f)
                    mfile['isnew'] = False
                    #print("merge file: ", nfile)
                    dir_files.append(mfile)
                else:
                    # new file
                    if nfile:
                        nfile['name'] = re.sub("\.h", ".c", f)
                        nfile['isnew'] = True
                        #print("new file: ", nfile)
                        dir_files.append(nfile)

            dir['files'] = dir_files
            self.dirs.append(dir)

    def __gen_func_return(self, rt_type):
        if rt_type in DEFAULT_RETURN.keys():
            return  DEFAULT_RETURN[rt_type]
        else:
            return "0"
       
    def __gen_scripts(self):
        print("start to generate scripts")

        # platform_config.cmake
        if not os.path.exists(self.target_path + "/platform_config.cmake"):
            print("    cp platform_config.cmake template")
            shutil.copy(self.porting_template_path + "platform_config.cmake", self.target_path + "/platform_config.cmake")

        # toolchain_file.cmake
        if not os.path.exists(self.target_path + "toolchain_file.cmake"):
            print("    cp toolchain_file.cmake template")
            shutil.copy(self.porting_template_path + "toolchain_file.cmake", self.target_path + "/toolchain_file.cmake")
        
        # kconfig
        if not os.path.exists(self.target_path + "Kconfig"):
            print("    cp Kconfig template")
            shutil.copy(self.porting_template_path + "Kconfig", self.target_path + "/Kconfig")
                
        # Linux first use template build.sh and makefile
        # print(self.abilitys)
        if not os.path.exists(self.target_path + "build_example.sh"):
            print("    cp build_example.sh template")
            shutil.copy(self.porting_template_path + "build_example.sh", self.target_path + "/build_example.sh") 
        
        if self.abilitys['OPERATING_SYSTEM'] == '100':
                print("    cp makefile")
                shutil.copy(self.porting_template_linux_path + "Makefile", self.target_path + "/Makefile")
           

    def __gen_adapter_default(self):
        if not os.path.exists(self.target_tuyaos_adapter_path):
            print("    create tuyaos_adapter dir")
            os.makedirs(self.target_tuyaos_adapter_path, exist_ok=True)

        # include directory, can be empty, kernel header files can be put in during component management in the development environment
        if not os.path.exists(self.target_tuyaos_adapter_path + "include"):
            print("    create include dir")
            os.makedirs(self.target_tuyaos_adapter_path + "include", exist_ok=True)

        # src directory, used for auto-generated framework code
        if not os.path.exists(self.target_tuyaos_adapter_path + "src"):
            print("    create src dir")
            os.makedirs(self.target_tuyaos_adapter_path + "src", exist_ok=True)

        # # Must update: local.mk & tkl_adapter.h/c
        # if not os.path.exists(self.target_tuyaos_adapter_path + "local.mk"):            
        #     print("    update file: local.mk")
        #     if self.template:
        #         if self.template == 'bsp':
        #             shutil.copy(self.porting_template_bsp_path + "local.mk", self.target_tuyaos_adapter_path + "/local.mk") 
        #             shutil.copy(self.porting_template_bsp_path + "tkl_adapter.h", self.target_tuyaos_adapter_path + "/include/tkl_adapter.h") 
        #             shutil.copy(self.porting_template_bsp_path + "tkl_adapter.c", self.target_tuyaos_adapter_path + "/src/tkl_adapter.c") 
        #         else:
        #             shutil.copy(self.porting_template_path + "local.mk.template", self.target_tuyaos_adapter_path + "/local.mk")   
        #     else:
        #         shutil.copy(self.porting_template_path + "local.mk.template", self.target_tuyaos_adapter_path + "/local.mk")    

    def __gen_adapter(self):
        print("start to generate tuyaos adapter")
        self.__gen_adapter_default()

        for d in self.dirs:
            print("    make ability:", d['name'])
            for f in d['files']:
                # If security-related configurations are not enabled, security-related adapter files can be not generated to avoid compilation issues.
                if f['name'] == "tkl_asymmetrical.c":
                    if 'ENABLE_PLATFORM_RSA' not in self.abilitys and  'ENABLE_PLATFORM_ECC' not in self.abilitys:
                        continue

                if f['name'] == "tkl_symmetry.c":
                    if 'ENABLE_PLATFORM_AES' not in self.abilitys:
                        continue

                if f['name'] == "tkl_hash.c":
                    if 'ENABLE_PLATFORM_SHA256' not in self.abilitys and 'ENABLE_PLATFORM_SHA1' not in self.abilitys and 'ENABLE_PLATFORM_MD5' not in self.abilitys:
                        continue

                if (f['isnew']):                    
                    # New file, if Linux has a template, use the template
                    if self.template_path:
                        if os.path.exists(self.template_path + f['name']):
                            print("        new file:",  f['name'] + ", use template")
                            shutil.copy(self.template_path + f['name'], self.target_tuyaos_adapter_path + "/src/")
                            continue                       

                    # If there is no template, regenerate
                    print("        new file:",  f['name'])
                else:
                    # Update old header files (may have new interfaces), to be opened after the kernel is officially built
                    print("        merge file: ", f['name'])

                fd = open(self.target_tuyaos_adapter_path + "/src/" + f['name'], 'w+')
                # C file header
                desc = re.sub("###FILE###", f['name'], file_desc)
                fd.write(desc)
                fd.write("\n\n")

                # include
                fd.write(udf_flags_begin)   # Custom .h
                if not f['udfs']:
                    fd.write("#include \"%s\"\n"%(re.sub("\.c", ".h", f['name']))) # Built-in .h
                    fd.write("#include \"tuya_error_code.h\"\n") # Built-in .h
                else:
                    for inc in f['udfs']:
                        fd.write(inc)
                fd.write(udf_flags_end + "\n")
                fd.write("\n")

                # funcs
                for func in f['funcs']:
                    #print(func)
                    fd.write(func['head'] + "\n")
                    fd.write("{\n")
                    if func['isnew']:                        
                        fd.write(func_body_flags_begin + "\n")
                        fd.write("    return %s;\n"%self.__gen_func_return(func['return']))                        
                        fd.write(func_body_flags_end + "\n")
                    else:
                        if func['body']:
                            fd.write("    " + func['body'] + "\n")
                        else:
                            fd.write(func_body_flags_begin + "\n")
                            fd.write("    return %s;\n"%self.__gen_func_return(func['return']))                        
                            fd.write(func_body_flags_end + "\n")
                    fd.write("}\n\n")

                fd.close()
        pass

    def gencode(self):
        print("generate code start!")
        if not os.path.exists(self.target_tuyaos_path): # New platform, need to create directories
            os.makedirs(self.target_tuyaos_path, exist_ok=True) 
            pass
        
        self.__gen_scripts()
        self.__gen_adapter()
        print("generate code finished!")


if __name__ == "__main__":
    # for root, sub, files in os.walk("./"):
    #     for f in files:
    #         print(os.path.join(root, f))

    if not sys.argv[1] or not sys.argv[2]:
        print("Usage: python3 ./kernel_porting.py path target")
        print("       example: python3 ./kernel_porting.py ./vendor/ fh8636-rtos")
    else:
        print(sys.argv)
        if len(sys.argv)<4:
            porting(sys.argv[1], sys.argv[2]).gencode()
        else:
            porting(sys.argv[1], sys.argv[2], sys.argv[3]).gencode()

