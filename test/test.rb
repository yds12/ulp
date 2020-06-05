ERROR_COLOR = "\033[31m"
SUCCESS_COLOR = "\033[32m"
STRONG_ERROR_COLOR = "\033[1;31m"
STRONG_SUCCESS_COLOR = "\033[1;32m"
END_COLOR = "\033[m"
EXTENSION = ".ul"
TEST_EXEC = "testexec"
BUILD_DIR = "build"

$total = 0
$success = 0

def run_tests dir, suite_label, expected_result
  puts suite_label

  files = `ls #{dir}/*#{EXTENSION}`.split "\n"


  files.each do |f|
    $total += 1
    command = "#{BUILD_DIR}/ulpc --silent #{f} -o #{BUILD_DIR}/#{TEST_EXEC}" \
      " ; echo $?"

  #  puts "Running command:"
  #  puts "\t#{command}"
  #  puts ""

    result = `#{command}`
    filename = f.sub "#{dir}/", ""
    
    if result.strip == expected_result then 
      $success += 1
      puts "\t#{SUCCESS_COLOR}pass#{END_COLOR} #{filename}"
    else 
      puts "\t#{ERROR_COLOR}fail#{END_COLOR} #{filename}" 
    end
  end

  puts ""
end

def print_totals
  failures = $total - $success

  print "#{$total} tests, "

  if $success == 0 then
    print "#{$success} passes"
  else
    print "#{STRONG_SUCCESS_COLOR}#{$success} passes#{END_COLOR}"
  end

  if failures == 0 then
    print " and #{failures} failures.\n"
  else
    print " and #{STRONG_ERROR_COLOR}#{failures} failures#{END_COLOR}.\n"
  end
end

print "Running test suite...\n\n"

run_tests "test/cases/pos", "Positive tests:", "0"
run_tests "test/cases/neg", "Negative tests:", "1"
print_totals
