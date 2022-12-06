
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "arduinoFFT.h"
 
LiquidCrystal_I2C lcd(0x27, 16, 2);

//Sprites do LCD

byte Vazio[] = {  
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};
 
byte Cheio[] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
 
byte metadeDireita[] = {
  B00011,
  B00011,
  B00011,
  B00011,
  B00011,
  B00011,
  B00011,
  B00011
};
 
byte metadeEsquerda[] = {
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000
};

#define AMOSTRAS 128 //Quantidade de capturas analógicas para formar uma frequência
#define BOTAO_1 7
#define BOTAO_2 8
#define LED 9
#define SOM 11
 
char printCordas[6][11] = {{"Corda 1 E4"}, {"Corda 2 B3"}, {"Corda 3 G3"}, {"Corda 4 D3"}, {"Corda 5 A2"}, {"Corda 6 E2"}};
int escolhaCorda = 0, posicao;
 
unsigned int periodo;
unsigned long microSeg;
int notas[] = {330, 247, 196, 147, 110, 82}; //Frequências da afinação padrão de um violão/guitarra
double micOut[AMOSTRAS];
double vZero[AMOSTRAS];
arduinoFFT FFT = arduinoFFT();
 
int posicaoAnt = 0;
 
double Frequencia(const int notas[],const int escolhaCorda){
  //**
  double freqMax;
 
  freqMax = notas[escolhaCorda] * 2.5; //Define a amostragem de Nyquist, que precisa ser 2x maior do que a frequência máxima esperada, sendo essa 1.25x maior do que o valor afinado
  periodo = round(1000000*(1.0/freqMax)); //
  for(int i=0; i<AMOSTRAS; i++){

        microSeg = micros();  //Retorna o numéro de microsegundos a partir do momento que o arduino roda a parte atual 
     
        micOut[i] = analogRead(0); //Guarda no array de saídas do microfone, a saída atual do microfone
        vZero[i] = 0; //Cria um vetor de zeros
 
        //Tempo de espera até a próxima coleta
        while(micros() < (microSeg + periodo)){
          ;
        }
        
    }
 
    //Aplicando Fourier às leituras do sensor
    FFT.Windowing(micOut, AMOSTRAS, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(micOut, vZero, AMOSTRAS, FFT_FORWARD);
    FFT.ComplexToMagnitude(micOut, vZero, AMOSTRAS);
 
    //Encontra o pico de frequência
    double picoFreq = FFT.MajorPeak(micOut, AMOSTRAS, freqMax);
    
    return picoFreq;
}
 
int PosicaoBarra(double Frequencia, int escolhaCorda){
  Frequencia *= 0.99; //O sensor tende a retornar valores um pouco acima da frequência real
  //O sensor retorna valores inesperados, porém padronizado, em baixas frequências, nesse caso 82hz
  if(escolhaCorda == 5){
    //A frequencia lida nesse caso tende a ser um pouco menos da metade do que a frequência real
    Frequencia *= 2.07;
  }
 
  Serial.println(Frequencia);

  //Calcula uma diferença em porcentagem da da frequência captada e da frequência afinada
  double dif = Frequencia/notas[escolhaCorda];
  //Proporção que irá indicar em qual posição ele será printado na tela lcd de acordo com o quão desafinado está
  double prop = 0.02;
 
  //Abaixo de 0.7 em geral é um ruido
  if(dif < 0.7){
    return -1;
  }

  //7 é a posição para afinado
  int a = 7;
 
  //Um valor afinado para o código está entre 0,6% mais grave ou 1% mais agudo
  if(dif < 0.9940){
    a = 6;
    while(dif < 0.9940){
      a--;
      dif += prop;
      if(a == 0){
        break;
      }
    }
  }
  else if(dif > 1.01){
    a = 9;
    while(dif > 1.01){
      a++;
      dif -= prop;
      if(a == 15){
        break;
      }
    }
  }
  else{
    return a;
  }
 
  //Os valores mais graves também apresentam um comportamento contrário do resto, sendo necessário reverter novamente sua posição
  if(escolhaCorda == 5){
    a -= 15;
    a *= -1;
  }
 
  return a;
 
}
 
void retornaTela(int botaoEsq, int botaoDir, int *telaAtual){
 
 int umaVez = 0;
 

 //O codigo seguinte faz com que o programa apenas continue quando o botão deixe de ser pressionado
  if(botaoEsq){
    while(botaoEsq){
      if(!umaVez){
        if(*telaAtual == 0){
          *telaAtual = 5;
        }
        else{
          *telaAtual = *telaAtual - 1;
        }
        umaVez = 1;
      }
      botaoEsq = digitalRead(BOTAO_1);
    }
  }  
  umaVez = 0;
  if(botaoDir){  
    while(botaoDir){
      if(!umaVez){
        if(*telaAtual == 5){
          *telaAtual = 0;
        }
        else{
          *telaAtual = *telaAtual + 1;
        }
 
        umaVez = 1;
      }
      botaoDir = digitalRead(BOTAO_2);
    }
  }
 
  umaVez = 0;
 
}
 
void setup() {
  Serial.begin(9600);

  //Setando os pinos
  pinMode(BOTAO_1, INPUT);
  pinMode(BOTAO_2, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(SOM, INPUT);
 

  //Configurando o LCD e os sprites
  lcd.backlight();
  lcd.begin(16, 2);
  lcd.createChar(0, metadeEsquerda);
  lcd.createChar(1, metadeDireita);
  lcd.createChar(2, Cheio);
  lcd.createChar(3, Vazio);
  lcd.home();
 
}
 
void loop() {

  //Captura dos botões
  int botaoEsquerda, botaoDireita, captandoSom;
  botaoEsquerda = digitalRead(BOTAO_1);
  botaoDireita = digitalRead(BOTAO_2);
  /*O sensor possui uma saída digital que indica caso um som foi captado ou não,
  tal comportamento depende da sensibilidade regulada pelo potenciometro do sensor*/
  captandoSom = digitalRead(SOM);
 
  //Se precionado o botão o que está escrito será apagado para escrever a tela selecionada
  if(botaoEsquerda || botaoDireita){
    retornaTela(botaoEsquerda, botaoDireita, &escolhaCorda);
    lcd.setCursor(posicaoAnt, 1);
    lcd.write(3);
    lcd.setCursor(posicaoAnt+1, 1);
    lcd.write(3);
    digitalWrite(LED, LOW);
 
  }
 
  lcd.setCursor(3, 0);
  lcd.print(printCordas[escolhaCorda]);
 
  //Caso um som seja captado, ele retornara o quao desafinado está o som captado através da barra printada na tela
  if(captandoSom){
  posicao = PosicaoBarra(Frequencia(notas, escolhaCorda), escolhaCorda);
  }
 
  /*Caso a posição seja diferente da já escrita na tela a barra anterior será apagada juntamente com a imediatamente
  a sua frente(afim de evitar problemas com o caractere especial)*/
  if(posicaoAnt != posicao && posicao != -1){
    lcd.setCursor(posicaoAnt, 1);
    lcd.write(3);
    lcd.setCursor(posicaoAnt+1, 1);
    lcd.write(3);
    posicaoAnt = posicao;
  }

  //Caso a frequência seja de acordo com a afinação sera escrito um caractere especial e o led acenderá
  if(posicao == 7 && posicao != -1){
    lcd.setCursor(7, 1);
    lcd.write(1);
    lcd.setCursor(8, 1);
    lcd.write(0);
    digitalWrite(LED, HIGH);
  }
  //Caso contrário a posição relativa sera escrita
  else if(posicao != -1){
    lcd.setCursor(posicao, 1);
    lcd.write(2);
    digitalWrite(LED, LOW);
  }
 
  delay(100);
 
}
