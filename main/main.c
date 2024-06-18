//////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                       _              //
//               _    _       _      _        _     _   _   _    _   _   _        _   _  _   _          //
//           |  | |  |_| |\| |_| |\ |_|   |\ |_|   |_| |_| | |  |   |_| |_| |\/| |_| |  |_| | |   /|    //    
//         |_|  |_|  |\  | | | | |/ | |   |/ | |   |   |\  |_|  |_| |\  | | |  | | | |_ | | |_|   _|_   //
//                                                                                       /              //
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
--     _____    ____    _____    _____    _____    ____       _____    ____    ______   _____    ______   --
--    / ____|  / __ \  |  __ \  |_   _|  / ____|  / __ \     / ____|  / __ \  |  ____| |  __ \  |  ____|  --
--   | |      | |  | | | |  | |   | |   | |  __  | |  | |   | |      | |  | | | |__    | |__) | | |__     --
--   | |      | |  | | | |  | |   | |   | | |_ | | |  | |   | |      | |  | | |  __|   |  _  /  |  __|    --
--   | |____  | |__| | | |__| |  _| |_  | |__| | | |__| |   | |____  | |__| | | |      | | \ \  | |____   --
--    \_____|  \____/  |_____/  |_____|  \_____|  \____/     \_____|  \____/  |_|      |_|  \_\ |______|  --
--                                                                                                        --                                                                                                
*/

//Área de inclusão de bibliotecas
//-----------------------------------------------------------------------------------------------------------------------

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ioplaca.h"  
#include "lcdvia595.h"
#include "driver/adc.h"
#include "hcf_adc.h"
#include "MP_hcf.h"  
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "string.h"

// Área das macros
//-----------------------------------------------------------------------------------------------------------------------
#define DESLIGAR_TUDO       saidas&0b00000000
#define LIGAR_RELE_1        saidas|0b00000001
#define DESLIGAR_RELE_1     saidas&0b11111110
#define LIGAR_RELE_2        saidas|0b00000010
#define DESLIGAR_RELE_2     saidas&0b11111101
#define LIGAR_TRIAC_1       saidas|0b00000100
#define DESLIGAR_TRIAC_1    saidas&0b11111011
#define LIGAR_TRIAC_2       saidas|0b00001000
#define DESLIGAR_TRIAC_2    saidas&0b11110111
#define LIGAR_TBJ_1         saidas|0b00010000
#define DESLIGAR_TBJ_1      saidas&0b11101111
#define LIGAR_TBJ_2         saidas|0b00100000
#define DESLIGAR_TBJ_2      saidas&0b11011111
#define LIGAR_TBJ_3         saidas|0b01000000
#define DESLIGAR_TBJ_3      saidas&0b10111111
#define LIGAR_TBJ_4         saidas|0b10000000
#define DESLIGAR_TBJ_4      saidas&0b01111111


#define TECLA_1 le_teclado() == '1'
#define TECLA_2 le_teclado() == '2'
#define TECLA_3 le_teclado() == '3'
#define TECLA_4 le_teclado() == '4'
#define TECLA_5 le_teclado() == '5'
#define TECLA_6 le_teclado() == '6'
#define TECLA_7 le_teclado() == '7'
#define TECLA_8 le_teclado() == '8'
#define TECLA_0 le_teclado() == '0'

#define STORAGE_NAMESPACE "storagenvs"
#define SENHA_KEY "senhanvs"
#define ADCMAX_KEY "adcmaxnvs"
#define ADCMIN_KEY "adcminnvs"

// Área de declaração de variáveis e protótipos de funções
//-----------------------------------------------------------------------------------------------------------------------

static const char *TAG = "Placa";
static uint8_t entradas, saidas = 0; //variáveis de controle de entradas e saídas

uint32_t adcvalor = 0;

int ctrl = 0;
int n1 = 0;
int qdig = 0;
int coluna = 0;
int encher = 0;
int molho = 0;
int bater = 0;
int exaguar = 0;
int ex1 = 0;
int ex2 = 0;
int ex3 = 0;
int ex4 = 0;
int centrifugar = 0;

char operador;
char tecla;
char mostra[40];

// Funções e ramos auxiliares
//-----------------------------------------------------------------------------------------------------------------------


// Programa Principal
//-----------------------------------------------------------------------------------------------------------------------

void app_main(void)
{   
    MP_init(); // configura pinos do motor
    // a seguir, apenas informações de console, aquelas notas verdes no início da execução
    ESP_LOGI(TAG, "Iniciando...");
    ESP_LOGI(TAG, "Versão do IDF: %s", esp_get_idf_version());

    /////////////////////////////////////////////////////////////////////////////////////   Inicializações de periféricos (manter assim)
    
    // inicializar os IOs e teclado da placa
    ioinit();      
    entradas = io_le_escreve(saidas); // Limpa as saídas e lê o estado das entradas

    // inicializar o display LCD 
    lcd595_init();
    lcd595_write(1,3,"Jornada 1");
    lcd595_write(2,0,"Programa Basico");
    
    // Inicializar o componente de leitura de entrada analógica
    esp_err_t init_result = hcf_adc_iniciar();
    if (init_result != ESP_OK) {
        ESP_LOGE("MAIN", "Erro ao inicializar o componente ADC personalizado");
    }

    // inica motor
    DRV_init(6, 7);

    //delay inicial
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
    lcd595_clear();

    /////////////////////////////////////////////////////////////////////////////////////   Periféricos inicializados
    
    lcd595_clear();
    lcd595_write(1,1, "Gab Palazini"); // apresentação 
    lcd595_write(2,1, "MAQ DE LAVAR"); // " "

    vTaskDelay(1500 / portTICK_PERIOD_MS);

    lcd595_clear();

    /////////////////////////////////////////////////////////////////////////////////////   Início do ramo principal                    
    while(1)
    {
        hcf_adc_ler(&adcvalor); // coleta o valor do poteciômetro, que será necessário em varios momentos do código

        entradas = io_le_escreve(saidas);

        tecla = le_teclado(); // atribui à variável tecla o valor pressionado no teclado através da função le_teclado

        if(ctrl == 0) // redundânica da if ctrl == 0 pois a outro comando semelhante anteriormente quando ctrl == 1, evitando sobreposições
        {
            n1 = n1 * 10 + tecla - '0'; // faz que n1 se torne o valor anterior de n1 (inicialmente 0) vezes 10 mais a tecla pressionada no teclado
        }

        if(tecla == '9')
        {   
            lcd595_clear();

            encher = 1;
            while(encher == 1)
            {
                lcd595_write(1,2, "Enchendo...");
                entradas = io_le_escreve(LIGAR_TRIAC_1);

                if(((entradas>>2)&1 && (entradas>>5)&1))
                {   
                    entradas = io_le_escreve(DESLIGAR_TRIAC_1);
                    molho = 1;
                    encher = 0;
                }
            }
        }
        
        if(molho == 1)
        {
            lcd595_clear();
             for (int i = 10; i >= 0; i--)
            {
                char contagem[17];
                sprintf(contagem, "Em molho por %ds", i); 
                lcd595_write(1, 0, contagem);              
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                if(i == 0)
                {   
                    bater = 1;
                    lcd595_clear();
                    molho = 0;
                }
            }
        }

        if(bater == 1)
        {
            lcd595_write(1,0, "Batendo...");
            for (int i = 10; i >= 0; i--)
            {
                entradas = io_le_escreve(LIGAR_RELE_1);
                vTaskDelay(250 / portTICK_PERIOD_MS);
                entradas = io_le_escreve(DESLIGAR_RELE_1);
                vTaskDelay(250 / portTICK_PERIOD_MS);
                
                if(i == 0)
                {
                exaguar = 1;
                lcd595_clear();
                bater = 0;
                }
            }
        }

        if(exaguar == 1)
        {
            if(ex2 != 1)
            {
            lcd595_write(1,0, "Enxaguando");
            entradas = io_le_escreve(LIGAR_TRIAC_2);
            lcd595_write(2,0, "Esvazinado");
            }
            
            if(((entradas>>2)&1) == 0 && ((entradas>>5)&1) == 0 && ex3 != 1)
            {
                lcd595_clear();
                lcd595_write(1,0, "Enxaguando");
                lcd595_write(2,0, "Enchendo");
                entradas = io_le_escreve(DESLIGAR_TRIAC_2);
                entradas = io_le_escreve(LIGAR_TRIAC_1);
                ex2 = 1;
            }

            if(((entradas>>2)&1) == 1 && ((entradas>>5)&1) == 1 && ex2 == 1 && ex1 != 1)
            {
                ex3 = 1;
                lcd595_clear();
                lcd595_write(1,0, "Enxaguando");
                lcd595_write(2,0, "Esvazinado");
                entradas = io_le_escreve(DESLIGAR_TRIAC_1);
                entradas = io_le_escreve(LIGAR_TRIAC_2);
            }

            if(((entradas>>2)&1) == 0 && ((entradas>>5)&1) == 0 && ex3 == 1) 
            {
                ex1 = 1;
            }

            if(ex1 == 1)
            {
                entradas = io_le_escreve(DESLIGAR_TRIAC_2);
                centrifugar = 1;
                ex1 = 0;
                exaguar = 0;
            }
        }

        if(centrifugar == 1)
        {
            lcd595_clear();
            lcd595_write(1,0, "Centrifugando");
            entradas = io_le_escreve(LIGAR_RELE_2);
            vTaskDelay(4500 / portTICK_PERIOD_MS);
            entradas = io_le_escreve(DESLIGAR_RELE_2);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            lcd595_clear();
            centrifugar = 0;
            ex1 = 0;
            ex2 = 0;
            ex3 = 0;
            ex4 = 0;
        }
    
        vTaskDelay(100 / portTICK_PERIOD_MS); // delay para a while
       
    }     
        
    hcf_adc_limpar(); // caso erro no programa, desliga o módulo ADC
}