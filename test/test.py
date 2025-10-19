import subprocess as s

out=s.run(["../A80/release/a80"], stdout=s.PIPE, stderr=s.STDOUT)
print (out.returncode)
print ("First basic test stdout.")
print(out.stdout)
print ("First basic test stderr.")
print(out.stderr)

#out=s.run(["../A80/release/a80", "-8085", "source/all8085.asm"], stdout=s.PIPE, stderr=s.STDOUT)
out=s.run(["../A80/release/a80", "source/all8085.asm"], stdout=s.PIPE, stderr=s.STDOUT)
print (out.returncode)
print ("Now compiling. Showing stdout.")
print(out.stdout)
print ("Now compiling. Showing stderr.")
print(out.stderr)

