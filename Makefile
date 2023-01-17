#MFBdjvu-1.0
#
#Based on djvulibre (http://djvu.sourceforge.net/)
#Based on monday2000 (http://djvu-soft.narod.ru/)
#Based on DjVu Thresholding Binarization (http://djvu-soft.narod.ru/bookscanlib/034.htm)
#Based CLI of simpledjvu (https://github.com/mihaild/simpledjvu).
#
#MFBdjvu is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with Simpledjvu.  If not, see <http://www.gnu.org/licenses/>.

PROJECT = mfbdjvu
DJVULIBRE_PATH = src/djvulibre
CXX = g++ -O3 -std=c++0x
INCLUDES = -I$(DJVULIBRE_PATH) -I$(DJVULIBRE_PATH)/libdjvu -I$(DJVULIBRE_PATH)/tools -Isrc
CXXFLAGS = $(INCLUDES) -DHAVE_CONFIG_H -pthread -DTHREADMODEL=POSIXTHREADS
LDFLAGS = -ldjvulibre
LN = $(CXX) -DHAVE_CONFIG_H
RM = rm -f

OBJ_FILES = src/mfbdjvu.o src/pgm2jb2.o src/jb2tune.o src/jb2cmp/libjb2cmp.a
BIN_FILES = $(PROJECT)

all: djvulibre_config $(BIN_FILES)

$(PROJECT): $(OBJ_FILES)
	$(LN) $^ $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/jb2cmp/libjb2cmp.a: 
	cd src/jb2cmp && ${MAKE}

djvulibre_config:
	cd src/djvulibre && ./autogen.sh

clean:
	$(RM) $(OBJ_FILES) $(OBJ_FILE_PGM) $(BIN_FILES)
	cd src/jb2cmp && ${MAKE} clean
