#include "Person.h"
#include <iostream>

using namespace std;

// Default constructor
Person::Person()
{
    name = "";
    age = 0;
}

// Parameterized constructor
Person::Person(string n, int a)
{
    name = n;
    age = a;
}

// Display basic person details
void Person::display()
{
    cout << "Name: " << name << endl;
    cout << "Age: " << age << endl;
}

// Getters
string Person::getName() const
{
    return name;
}

int Person::getAge() const
{
    return age;
}

// Setters
void Person::setName(string n)
{
    name = n;
}

void Person::setAge(int a)
{
    age = a;
}