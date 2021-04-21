# Create a header file with the VERSION

if [ "$#" -ne 2 ]
then
  echo "Usage: version_header.sh <path> <version>"
  exit 1
fi

cat > $1/src/utility/version.h << END
/* version.h
 * File created automatically by CMakeLists.txt
 * Kept in sync with VERSION file
 */
std::string const VERSION="$2";
END
