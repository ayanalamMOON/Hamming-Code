#include<iostream>
#include<cmath>
using namespace std;

int main() {
    int data[30], dataatrec[30]={0}, n, r=0, i, p;

    cout<<"Enter the length of data\n";
    cin>>n;

    cout<<"Enter data of "<<n<<" bits in a single line\n";
    for(i=0; i<n; i++)
        cin>>data[i];

    //Calculation of number of redundancy bits
    while(pow(2,r)<(n+r+1))
        r++;

    // Copy data bits to dataatrec
    for(i=n-1; i>=0; i--)
        dataatrec[i+r] = data[i];

    //Calculation of even parity
    for(i=0; i<r; i++){
        p=pow(2,i);
        for(int j=p-1; j<n+r; j+=2*p){
            for(int k=j; k<j+p && k<n+r; k++)
                dataatrec[p-1]^=dataatrec[k];
        }
    }

    cout<<"\nEncoded data is\n";
    for(i=0;i<n+r;i++)
        cout<<dataatrec[i];

    cout<<"\n\nEnter received data bits in a single line\n";
    for(i=0;i<n+r;i++)
        cin>>data[i];

    //Checking for error
    int error_pos = 0;
    for(i=0; i<r; i++){
        p=pow(2,i);
        int parity=0;
        for(int j=p-1; j<n+r; j+=2*p){
            for(int k=j; k<j+p && k<n+r; k++)
                parity^=data[k];
        }
        if(parity)
            error_pos+=p;
    }

    if(error_pos==0) {
        cout<<"\nNo error while transmission of data\n";
    }
    else {
        cout<<"\nError on position "<<error_pos;

        cout<<"\nData sent : ";
        for(i=0;i<n+r;i++)
            cout<<dataatrec[i];

        cout<<"\nData received : ";
        for(i=0;i<n+r;i++)
            cout<<data[i];

        cout<<"\nCorrect message is\n";

        //if errorneous bit is 0 we complement it else vice versa
        if(data[error_pos-1]==0)
            data[error_pos-1]=1;
        else
            data[error_pos-1]=0;
        for (i=0;i<n+r;i++) {
            cout<<data[i];
        }
    }

    return 0;
}