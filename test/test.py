import subprocess as sp
import sys

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

program_directory: str = ""
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

    a80_path = program_directory + "/a80"
    source_path = source_directory + "/all8085.asm"
    
    out=sp.run([a80_path, "-8085", source_path], stdout=sp.PIPE, stderr=sp.STDOUT)

    decoded_lines = out.stdout.decode('utf-8').splitlines()
    for line in decoded_lines:
        print("   " + line)

    if (out.returncode != 0):
        print("   Returned {0}, 0 expected.".format(out.returncode))
        return False

    return True
        
def test_no_sim_rim_in_8080():
    print("\n*** Testing we do not have RIM and SIM in 8080.")

    a80_path = program_directory + "/a80"
    source_path = source_directory + "/all8085.asm"
    
    out=sp.run([a80_path, "-8080", source_path], stdout=sp.PIPE, stderr=sp.STDOUT)

    decoded_lines = out.stdout.decode('utf-8').splitlines()
    for line in decoded_lines:
        print("   " + line)

    if (out.returncode != 0):
        print("   Returned {0}, 0 expected.".format(out.returncode))
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
    if len(parameters) < 3:
        print("1st parameter: path for Assembler80 directory.")
        print("2nd parameter: path for directory with source files to be tested.")
        exit(1)

    global program_directory, source_directory
    program_directory = parameters[1]
    source_directory = parameters[2]

    count : int = run_tests()
    print("\n>>> Tests finished with {0} error(s)".format(count))

if __name__ == "__main__":
    main(sys.argv)
