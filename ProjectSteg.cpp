#include <opencv2/core/core.hpp>     
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
//opencv2 библеотека компьютерного зрения, которая предназначена для анализа, классификации и обработки изображения 

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
//импортируем библеотеки 

using namespace cv;
using namespace std;
//using namespace std сообщает компилятору что мы хотим использовать всё, что находится в пространстве имен "std"

char* progpath;

void usage() {
  cerr << "Usage: \n";
  cerr << "  " << progpath << " h <image path> <data path> <output image path>\n";
  cerr << "  " << progpath << " x <image path> <output data path>\n";
}
//h <image path> <data path> <output image path> команда для закодировки информации в изображение (вместо <xxx> нужно вставить путь к файлу)
//x <image path> <output data path> команда для раскодировки информации из изображения (вместо <xxx> нужно вставить путь к файлу)


void fatalerror(const char* error) {
  cerr << "ERROR: " << error << endl;
  usage();
  exit(1);
}


void info(const char* msg) {
  cerr << "[+] " << msg << endl;
}

void hide_data(char* inimg, char* indata, char* outimg);
void extract_data(char *inimg, char* outdata);

int main( int argc, char** argv ) {
  progpath = argv[0];
  if ( argc < 2 ) {
    fatalerror("No arguments passed");
  }
  if ( argv[1][1] != '\0' ) {
    fatalerror("Operation must be a single letter");
  }
  if ( argv[1][0] == 'h' ) {
    if ( argc != 5 ) {
      fatalerror("Wrong number of parameters for [h]ide operation");
    }
    hide_data(argv[2], argv[3], argv[4]);
  } else if ( argv[1][0] == 'x' ) {
    if ( argc != 4 ) {
      fatalerror("Wrong number of parameters for e[x]tract operation");
    }
    extract_data(argv[2], argv[3]);
  } else {
    fatalerror("Unknown operation");
  }
  return 0;
}
//функция которая показывет что было сделано не так "No arguments passed"- аргументы не переданы и тд...

Mat calc_energy(Mat img) {
  Mat orig;
  Mat shifted;
  Mat diff;
  Mat res;

  bitwise_and(img, 0xF0, img);

  copyMakeBorder(img, orig, 1, 1, 1, 1, BORDER_REPLICATE);

  res = Mat::zeros(orig.size(), orig.type());

  int top[8] = {1,0,0,0,1,2,2,2};
  int left[8] = {2,2,1,0,0,0,1,2};
  for ( int i = 0 ; i < 8 ; i++ ) {
    copyMakeBorder(img, shifted, top[i], 2-top[i], left[i], 2-left[i], BORDER_REPLICATE);
    absdiff(orig, shifted, diff);
    res = max(res, diff);
  }

  return res(Rect(1, 1, img.cols, img.rows)); // x, y, Ширина, Высота
}
// Изображение представляет собой двумерную матрицу, которая представлена в виде класса cv::Mat.
//Каждый элемент матрицы представляет собой один пиксель. 
//Для изображений в градациях серого элемент матрицы представлен 8-битным числом без знака (от 0 до 255).
//Для цветного изображения в формате RGB таких чисел 3, по одному на каждую компоненту цвета.


typedef pair<pair<uchar, int>, pair<int, pair<int, int> > > Energy;
//typedef функция определяющая новый тип данных, что в нашем случае "Energy"

inline Energy _energy(int r, int c, int ch, uchar v) {
  int nonce = ch * ch * 10666589 + r * r + c * c + 2239; //для равномерного распределения данных
  return make_pair(make_pair(v, nonce), make_pair(ch, make_pair(c, r)));
}

inline int _energy_r(const Energy &e) { return e.second.second.second; }
inline int _energy_c(const Energy &e) { return e.second.second.first; }
inline int _energy_ch(const Energy &e) { return e.second.first; }
inline int _energy_v(const Energy &e) { return e.first.first; }


vector<Energy> energy_order(Mat img) {
  /* Вяозвращает вектор в порядке убывания "энергии". */

  Mat energy = calc_energy(img.clone());

  info("Calculated energies");

  vector<Energy> energylist;

  for ( int r = 0 ; r < img.rows ; r++ ) {
    for ( int c = 0 ; c < img.cols ; c++ ) {
      const Vec3b vals = energy.at<Vec3b>(r,c);
      for ( int ch = 0 ; ch < 3 ; ch++ ) {
	uchar v = vals[ch];
	if ( v > 0 ) {
	  energylist.push_back(_energy(r,c,ch,v));
	}
      }
    }
  }

  sort(energylist.begin(), energylist.end());
  reverse(energylist.begin(), energylist.end());

  return energylist;
}
//определяет "энергию" загруженной фотографии

void write_into(Mat &img, vector<Energy> pts, char *buf, int size) {
  int written = 0;
  char val;
  int count = 0;
  for ( vector<Energy>::iterator it = pts.begin() ;
	it != pts.end() && written != size ;
	it++, count++ ) {
    uchar data;
    if ( count % 2 == 0 ) {
      val = buf[written];
      data = (val & 0xf0) / 0x10;
    } else {
      data = (val & 0x0f);
      written += 1;
    }

    Energy &e = *it;
    Vec3b &vals = img.at<Vec3b>(_energy_r(e), _energy_c(e));
    uchar &v = vals[_energy_ch(e)];
    v = (0xf0 & v) + data;
  }

  if ( written != size ) {
    fatalerror("Could not write all bytes");
  }
}
//если размер текста намного больше чем картинка может в себя поместить функция выдаст ошибку и сообщение "Could not write all bytes"

void read_from(Mat &img, vector<Energy> pts, char* buf, int size) {
  int read = 0;

  int count = 0;
  char val = 0;

  for ( vector<Energy>::iterator it = pts.begin() ;
	it != pts.end() && read != size ;
	it++, count++ ) {
    Energy &e = *it;
    const Vec3b val = img.at<Vec3b>(_energy_r(e), _energy_c(e));
    const uchar v = val[_energy_ch(e)];
    const uchar data = 0x0f & v;
    char out;
    if ( count % 2 == 0 ) {
      out = data * 0x10;
    } else {
      out += data;
      buf[read++] = out;
    }
  }

  if ( read != size ) {
    fatalerror("Wrong size");
  }
}

bool is_valid_image_path(char *path) {
  int l = strlen(path);
  return strcmp(path + l - 4, ".bmp") == 0 ||
    strcmp(path + l - 4, ".png") == 0;
}
//определяет расширение картинки

void hide_data(char* inimg, char* indata, char* outimg) {
  if ( !is_valid_image_path(outimg) ) {
    fatalerror("Output path must be either have .png or .bmp as extension.");
  }
//если расширение картинки не будет соответствовать нужному (либо ".png" , либо ".bmp")
  
  Mat img = imread(inimg, cv::IMREAD_COLOR);
  if ( ! img.data ) {
    fatalerror("Could not load image. Please check path.");
  }
  //при ошибки загрузки картинки выдаст сообщение "Could not load image. Please check path."
  info("Loaded image");
  //"Loaded image" вызванное сообщение в случае успешной загрузки

  ifstream fin(indata, ios_base::binary);
  if ( ! fin.good() ) {
    fatalerror("Could not read data from file. Please check path.");
  }
  //если функция не сможет прочесть информацию с файла выдаст ошибку "Could not read data from file. Please check path."

  char_traits<char>::pos_type fstart = fin.tellg();
  fin.seekg(0, ios_base::end);
  long int fsize = (long int) (fin.tellg() - fstart);
  fin.seekg(0, ios_base::beg);
  char *buf = new char[fsize + 16];
  memcpy(buf, "PROJECTSTEG", 8);
  memcpy(buf + 8, &fsize, 8);
  fin.read(buf + 16, fsize);
  fin.close();
  info("Read data");

  vector<Energy> pts = energy_order(img);
  info("Found energy ordering");

  write_into(img, pts, buf, fsize + 16);
  info("Updated pixel values");
//при записи текста в картинку функция обновляет инфу о значении пикселя 
  imwrite(outimg, img);
  info("Finished writing image");
//"Finished writing image" при успешном встраивании в биты картинки информации 
  delete[] buf;
}


//Функция экстракции информации (текста) с картинки
void extract_data(char *inimg, char* outdata) {
  Mat img = imread(inimg, cv::IMREAD_COLOR);
  if ( ! img.data ) {
    fatalerror("Could not load image. Please check path.");
  }
  info("Loaded image");

  vector<Energy> pts = energy_order(img);
  info("Found energy ordering");

  char header[16];
  read_from(img, pts, header, 16);
  if ( memcmp(header, "PROJECTSTEG", 8) != 0 ) {
    fatalerror("Not a PROJECTSTEG encoded image");
  }

  long int fsize;
  memcpy(&fsize, header+8, 8);
  info("Found data length");
//нахождение длины текста полученного с картинки 
  char *buf = new char[fsize + 16];
  read_from(img, pts, buf, fsize + 16);
  info("Loaded data from pixels");
//считывание информации с пикселей 

  ofstream fout(outdata, ios_base::binary);
  fout.write(buf + 16, fsize);
  fout.close();
  info("Finished writing data");

  delete [] buf;
}

//CV_LOAD_IMAGE_COLOR

//cv::IMREAD_COLOR