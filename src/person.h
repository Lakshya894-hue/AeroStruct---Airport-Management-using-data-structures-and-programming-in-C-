#ifndef PERSON_H
#define PERSON_H

#include <string>

using namespace std;

class Person
{
private:
    string name;
    int age;

public:
    Person();
    Person(string n, int a);

    virtual ~Person() = default;

    virtual void display();

    string getName() const;
    int getAge() const;

    void setName(string n);
    void setAge(int a);
};

#endif