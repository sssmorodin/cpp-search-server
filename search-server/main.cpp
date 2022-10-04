// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
#include <string>
#include <iostream>

using namespace std;

int main() {
    //инициализируем счетчик
    int counter = 0;

    for (int i = 1; i <= 1000; ++i) {

        // сделаем из числа строку, пройдем по элементам 
        // циклом range-based и сравним каждый элемент строки с 3
        string s = to_string(i);
        for (char& c : s) {
            if (c == '3') {
                ++counter;
                continue;
            }
        }
    }

    // вывод результата
    cout << counter << " чисел от 1 до 1000 содержат как минимум одну цифру 3"s << endl;
}
// Закомитьте изменения и отправьте их в свой репозиторий.
