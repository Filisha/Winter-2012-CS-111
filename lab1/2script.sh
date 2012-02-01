# This comment should be ignored
:   semicolon test
     : spaces should not matter
: : :
		 
#simple command tests
echo standard echo
(echo echo in shell)

# redirection tests
echo this is my first test file > testfiles
cat testfiles
echo overwrite file > testfiles
cat < testfiles
cat < FILEDOESNOTEXIST  #should produce an error
rm testfiles

