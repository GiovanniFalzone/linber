/*-------------------------------------------------------------------------------------------------------
	This is a simple Client application using the linber_client class to implement a client of a service
--------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "Calculator.pb.h"
#include "Calculator_service_client.h"

using namespace std;

int main(){
	Calculator_service_client calc;

	for(float i=0; i<10; i++){
		cout << "--------------"<< i <<"------------------" << endl;
		printf("(%f %c %f) = ", i, '+', i);
		cout << calc.sum(i, i) << endl;
		printf("(%f %c %f) = ", i, '-', i);
		cout << calc.difference(i, i) << endl;
		printf("(%f %c %f) = ", i, '*', i);
		cout << calc.product(i, i) << endl;
		printf("(%f %c %f) = ", i, '/', i);
		cout << calc.division(i, i) << endl;
		printf("(%f %c %f) = ", i, '^', i);
		cout << calc.pow(i, i) << endl;
		printf("(\\/%f) = ", i);
		cout << calc.sqrt(i) << endl;
	}

	return 0;
}