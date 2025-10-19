import subprocess as sp
import sys
import os

#out=s.run(["../A80/release/a80"], stdout=s.PIPE, stderr=s.STDOUT)
#print (out.returncode)
#print ("First basic test stdout.")
#print(out.stdout)
#print ("First basic test stderr.")
#print(out.stderr)
#
##out=s.run(["../A80/release/a80", "-8085", "source/all8085.asm"], stdout=s.PIPE, stderr=s.STDOUT)
#out=s.run(["../A80/release/a80", "source/all8085.asm"], stdout=s.PIPE, stderr=s.STDOUT)
#print (out.returncode)
#print ("Now compiling. Showing stdout.")
#print(out.stdout)
#print ("Now compiling. Showing stderr.")
#print(out.stderr)

a80_directory: str = ""
source_directory: str = ""

def assembly(include_path: str = "",
             output_file: str = "",
             create_sym_table: bool = False,
             symbol_table_file: str = "",
             use_pre_compiled_header: bool = False,
             cpu: str = "8085"):
    out = sp.run()
    
def test_all_8085_instructions():
    print("\n*** Testing all 8085 instuctions.")

    a80_path = a80_directory + "/a80"
    source_path = source_directory + "/all8085.asm"

    rel_file = os.path.basename(source_path)
    rel_file,_ = os.path.splitext(rel_file)
    rel_file = rel_file + ".rel"
    if (os.path.exists(rel_file)):
        os.remove(rel_file)
    
    print ("\n   Calling {0} -8085 {1}\n".format(a80_path, source_path))
    out=sp.run([a80_path, "-8085", source_path], stdout=sp.PIPE, stderr=sp.STDOUT)

    decoded_lines = out.stdout.decode('utf-8').splitlines()
    for line in decoded_lines:
        print("   " + line)

    if (out.returncode != 0):
        print("   Returned {0}, 0 expected.".format(out.returncode))
        return False

    l80_path = l80_directory + "/l80"
    print ("   Calling {0} all8085.hex {1}\n".format(l80_path, rel_file))
    out=sp.run([l80_path, "all8085.hex", rel_file], stdout=sp.PIPE, stderr=sp.STDOUT)

    if (out.returncode != 0):
        print("   Returned {0}, 0 expected.".format(out.returncode))
        return False

    return True
        
def test_no_sim_rim_in_8080():
    print("\n*** Testing we do not have RIM and SIM in 8080.")

    a80_path = a80_directory + "/a80"
    source_path = source_directory + "/all8085.asm"
    
    print ("   Calling {0} -8080 {1}\n".format(a80_path, source_path))
    out=sp.run([a80_path, "-8080", source_path], stdout=sp.PIPE, stderr=sp.STDOUT)

    decoded_lines = out.stdout.decode('utf-8').splitlines()
    for line in decoded_lines:
        print("   " + line)

    if (out.returncode != 1):
        print("   Returned {0}, 1 expected.".format(out.returncode))
        return False

    return True

def run_tests() -> int:
    count: int = 0    # Number of errors

    if not test_no_sim_rim_in_8080():
        count = count + 1
    if not test_all_8085_instructions():
        count = count + 1
    return count

def main(parameters: list[str]) -> None:
    if len(parameters) < 4:
        print("1st parameter: directory where the a80 executable is.")
        print("2nd parameter: directory where the l80 executable is.")
        print("3rd parameter: path for directory with source files to be tested.")
        exit(1)

    global a80_directory, l80_directory, source_directory
    a80_directory = parameters[1]
    l80_directory = parameters[2]
    source_directory = parameters[3]

    count : int = run_tests()
    print("\n>>> Tests finished with {0} error(s)".format(count))

if __name__ == "__main__":
    main(sys.argv)
