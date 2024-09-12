#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <stdint.h>


//Funcion dada por el profesor en clase para abrir y cerrar puerto
HANDLE openSerial(const char * puerto, uint32_t baud_rate){

    HANDLE  p = CreateFileA(puerto,GENERIC_READ | GENERIC_WRITE,0,
                            NULL,OPEN_EXISTING,0, NULL);
    if(p == INVALID_HANDLE_VALUE) {
        printf("Error al crear el puerto\n");
        return INVALID_HANDLE_VALUE;
    }

    BOOL ok = FlushFileBuffers(p);
    if(!ok){
        printf("Error al vaciar buffer\n");
        CloseHandle(p);
        return INVALID_HANDLE_VALUE;
    }

    COMMTIMEOUTS timeouts;
    GetCommTimeouts(p,&timeouts);
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier =0;
    timeouts.ReadIntervalTimeout =0;
    timeouts.WriteTotalTimeoutConstant =50;
    timeouts.WriteTotalTimeoutMultiplier =0;
    ok = SetCommTimeouts(p,&timeouts);
    if(!ok){
        printf("Error al cargar timeouts\n");
        CloseHandle(p);
        return INVALID_HANDLE_VALUE;
    }

    DCB estado;
    GetCommState(p,&estado);
    estado.BaudRate = baud_rate;
    estado.ByteSize = 8;
    estado.Parity = NOPARITY;
    estado.StopBits = ONESTOPBIT;
    ok = SetCommState(p,&estado);
    if(!ok){
        printf("Error al cargar estado\n");
        CloseHandle(p);
        return INVALID_HANDLE_VALUE;
    }


    return p;
}
int write_port(HANDLE port, uint8_t * buffer, size_t size){
    DWORD written;
    BOOL ok = WriteFile(port, buffer, size, &written, NULL);
    if (!ok){
        printf("Error al escribir puerto\n");
        return -1;    }
    if (written != size){
        printf("Error al escribir todos los bytes en el puerto\n");
        return -1;
    }
    return 0;
}

SSIZE_T read_port(HANDLE port, uint8_t * buffer, size_t size)
{
    DWORD received;
    BOOL ok = ReadFile(port, buffer, size, &received, NULL);
    if (!ok){
        printf("Error al leer del puerto\n");
        return -1;
    }
    return received;
}

//Declaramos la estructura de nuestro PID

struct LUCES{
    //Definimos el struct de Luces con lo que se pide
    uint8_t Numero_estados_luces;
    uint8_t Estados_luces[200];
    uint32_t Tiempo_luces;
};

//Funcion para obtener un nombre de archivo cuando lo necesito por ejemplo cuando quiero crear uno o cargar uno, no devuelve nada
void obtenernombrearchivo(char * nombre) {
    //Pido en pantalla un nombre
    printf("Introduzca el nombre: \n");
    //Lo leo, como maximo sera de 20 caracteres de grande, mas grande ya es raro
    fgets(nombre, 20, stdin);
    //Busco donde esta el salto de linea que le introduzco al escribirlo porque pulso intro al final y se guarda
    char *p = strchr(nombre, '\n');
    //Lo quito
    *p = '\0';
    //Le concateno al final del nombre ".txt" para que el archivo tenga dicha extensión y se pueda abrir con el bloc de notas
    strcat(nombre, ".txt");
}


//Funcion para mostrar por pantalla los parametros del PID que tenemos
void Mostrar_pantalla_luces(struct LUCES s){
    int contador=0;
    printf("Estados luces: \n\n");
    for (int i=0;i<s.Numero_estados_luces;i++){
        printf("Estado (%d): %hhu \n",i,s.Estados_luces[i]);
    }
};

//Funcion guardar luces
void saveFile(const char *nombreFichero, struct LUCES L) {
    //Abre un fichero con el nombre nombrefichero
    FILE *f = fopen(nombreFichero, "w");
    if (f){
        //Escribe los valores en el fichero determinado
        fprintf(f,"Numero de estados: %hhu\n",L.Numero_estados_luces);
        for(int i=0;i<L.Numero_estados_luces;i++){
            fprintf(f,"Estado (%d): %hhu\n",i,L.Estados_luces[i]);
        }
        fprintf(f,"Tiempo de paso de estado a estado: %d s\n",L.Tiempo_luces);
        //Cerramos el fichero
        fclose(f);
    }
};

//Funcion para crear un nuevo fichero con un determinado nombre inicializando por defecto todos los atrubutos del struct del pid a 0
void newFile(const char *nombreFichero, struct LUCES L) {
    L.Numero_estados_luces=8;
    for(int i=0;i<L.Numero_estados_luces;i++){
        L.Estados_luces[i]=0;
    }
    L.Tiempo_luces=500;
    //Grabamos el archivo
    saveFile(nombreFichero,L);
};

//Funcion para cargar valores de un archivo
void loadFile(const char *nombreFichero,struct LUCES L){
    int selection=0;
    //Abrimos el archivo con el nombre determinado
    FILE *f = fopen(nombreFichero, "r");
    if (f){
        //Vamos leyendo linea por linea del archivo y lo metemos en las variables
        char linea[100]; // Tamaño suficiente para una línea
        // Leer linea a linea
        fgets(linea, sizeof(linea), f);
        fprintf(f,"Numero de estados: %hhu\n",L.Numero_estados_luces);
        for(int i=0;i<L.Numero_estados_luces;i++){
            fgets(linea, sizeof(linea), f);
            sscanf(linea, "Estado (%d): %hhu\n",&i,&L.Estados_luces[i]);
        }
        fgets(linea, sizeof(linea), f);
        fprintf(f,"Tiempo de paso de estado a estado: %d s\n",L.Tiempo_luces);
    }else{
        //Si se introduce el nombre de un archivo que no existe da error y da la opcion de crear uno con ese nombre
        printf("Error, no existe ningun archivo con ese nombre\nDesea crear uno nuevo con ese nombre?\n\n1 Si\n2 No\n");
        //Leemos con el scanf
        scanf("%d",&selection);
        //MUY IMPORTANTE despues de cada scanf un fflush porque se quedan residuos en el buffer que afectan a los siguientes scanf y al puerto serie
        fflush(stdin);
        if (selection==1){
            newFile(nombreFichero,L);
        };
    }
}


int main() {

    struct LUCES L; //estructura LUCES
    char buf=0; //bufer auxiliar para control de errores
    char x=0; //variable control seleccion Menu principal
    char y=0; //variable control seleccion Menu parametros
    char z=0; //variable control seleccion Menu archivo
    char *archivotrabajo=""; //Nombre de archivo con el que se esta trabajando en el proyecto en un momento determinado
    char nombre[20] = ""; //Nombre auxiliar

    //Bucle para un menu, se sale cuando la variable de dicho menu, en este caso el del principal, la x, sea 0, para salir
    do{
        //Limpiamos pantalla
        system("cls");
        //Mostramos menu y las opciones
        printf("MENU PRINCIPAL \n\n Introzuzca: \n (1) Estados luces \n (2) Mostrar secuencia de luces\n (3) Archivo\n (4) Abrir puerto serie\n (0) Para salir\n");
        //Leemos las opciones
        scanf("%c%c",&x,&buf);
        fflush(stdin); //Importantisimo limpiar buffer despues del scanf porque interfiere con otros scanf al dejar cosas en el buffer
        //Seguridad ante la introduccion de letras o numeros no validos
        if ((x <= '4') && buf == '\n') {
            switch (x) {
                case '1':
                    //Menú de parámetros
                    do {
                        //Control de errores, miramos que el archivo de trabajo no esta vacio, es decir, que tenemos uno abierto y entonces podemos trabajar
                        if (strlen(archivotrabajo)){
                            system("cls");
                            printf("MENU SECUENCIA LUCES \n\nIntrozuzca: \n (1) Para crear una secuencia de luces \n (2) Para modificar una secuencia\n (0) Para volver al menu principal\n");
                            scanf("%c%c", &y, &buf);
                            fflush(stdin);
                            //Control de errores
                            if ((y <= '3') && buf == '\n') {
                                switch (y) {
                                    case '1':
                                        //Crear una secuencia de luces
                                        system("cls");
                                        printf("\n Introduzca el numero maximo de estados\n");
                                        scanf("%d",&L.Numero_estados_luces);
                                        fflush(stdin);
                                        for(int i=0;i<L.Numero_estados_luces;i++){
                                            printf("\n Introduzca el estado (%d) :\n",i);
                                            if (scanf("%hhu", &L.Estados_luces[i]) != 1) {
                                                fflush(stdin);
                                                printf("Error al leer el valor.\n");
                                            } else {
                                                printf("El estado (%d) es: %u\n",i, L.Estados_luces[i]);
                                            }
                                            system("pause");
                                            system("cls");
                                        }
                                        break;
                                    case '2': {
                                        //Modificar secuencias

                                        uint8_t sec_mod;
                                        printf("\n Introduzca el numero de la secuencia que quiera modificar: ");
                                        scanf("%hhu",&sec_mod);
                                        fflush(stdin);
                                        printf("\n Introduzca el numero valor de la secuencia (%hhu): ",sec_mod);
                                        scanf("%hhu",&L.Estados_luces[sec_mod]);
                                        break;
                                    }
                                    case '0':
                                        break;
                                }
                            } else {
                                //Control de errores
                                puts("Introduce un numero valido");
                                system("pause");
                            }
                        }else{
                            //Si intentamos modificar variables sin abrir un fichero
                            printf("No hay ningun fichero de trabajo seleccionado. Desea crear uno nuevo por defecto?\n\n(1)Si\n(2)No\n");
                            char seleccion ;
                            scanf("%c",&seleccion);
                            fflush(stdin);
                            if(seleccion=='1'&& buf == '\n'){
                                archivotrabajo="NuevoDocumentoLUCES.txt";
                                newFile(archivotrabajo, L);
                            }else if(seleccion=='2'&& buf == '\n'){
                                break;
                            }else{
                                //Si introduces algo no valido, control de errores
                                puts("Introduce un numero valido");
                                system("pause");
                                break;
                            }
                        }
                    } while (y != '0');
                    break;
                case '2':
                    //Mostrar los parámetros introducidos o cargados de archivo
                    system("cls");
                    Mostrar_pantalla_luces(L);
                    system("pause");
                    break;
                case '3':
                    //Gestion de archivos
                    do {
                        system("cls");
                        printf("MENU ARCHIVOS \n\nIntrozuzca: \n (1) Guardar \n (2) Guardar como\n (3) Cargar\n (4) Para crear un nuevo archivo\n (0) Para volver al menu principal\n");
                        scanf("%c%c", &z, &buf);
                        fflush(stdin);
                        if ((z <= '4') && buf == '\n') {
                            switch (z) {
                                case '1':
                                    //Guardar archivo
                                    if (strlen(archivotrabajo)) {
                                        saveFile(archivotrabajo,L);
                                    }else{
                                        puts("No hay ningun archivo abierto");
                                        system("pause");
                                    }
                                    break;
                                case '2':
                                    //Guardar datos en fichero con nombre nuevo

                                    //Obtiene nombre
                                    obtenernombrearchivo(nombre);
                                    //Asigna ese nombre al archivo de trabajo actual
                                    archivotrabajo=nombre;
                                    //Crea un fichero con ese nombre y guarda
                                    saveFile(nombre, L);
                                    break;
                                case '3':
                                    //Leer fichero y cargar datos
                                    obtenernombrearchivo(nombre);
                                    archivotrabajo=nombre;
                                    //Carga los valores
                                    loadFile(nombre,L);
                                    break;
                                case '4':
                                    printf("Desea introducir un nombre o dejar predefinido?\n(1)Introducir un nombre\n(2)Dejar nombre predefinido\n");
                                    char seleccion ;
                                    scanf("%c",&seleccion);
                                    fflush(stdin);
                                    if(seleccion=='1'&& buf == '\n'){
                                        //Obtiene nombre
                                        obtenernombrearchivo(nombre);
                                        //Asignamos el nombre al archivo de trabajo actual
                                        archivotrabajo=nombre;
                                        //Creamos un nuevo archivo
                                        newFile(archivotrabajo, L);
                                        //Lo cargamos a las variables del PID
                                        loadFile(archivotrabajo,L);
                                    }else if(seleccion=='2'&& buf == '\n'){
                                        //Actualiza nombre del archivo actual de trabajo
                                        archivotrabajo="NuevoDocumentoLUCES.txt";
                                        newFile(archivotrabajo, L);
                                        loadFile(archivotrabajo,L);
                                    }else{
                                        //Si se introduce algo no valido
                                        puts("Introduce un numero valido");
                                        system("pause");
                                    }
                                    break;
                                case '0':
                                    //salir

                                    break;
                            }
                        } else {
                            puts("Introduce un numero valido");
                            system("pause");
                        }
                    } while (z != '0');
                    break;
                case '4': {
                    //Se mira si tenemos algun archivo abierto para no trabajar con datos residuales del buffer
                    if (strlen(archivotrabajo)){
                        char U='\0';
                        char com[10];
                        int baudios=0;
                        //Leemos los baudios y el puerto seleccionados
                        printf("Ingrese el numero del COMX X=(1, 2, 3, 4, 5, 6, 7, 8): ");
                        fgets(com,sizeof(com),stdin);
                        com[strcspn(com, "\n")] = '\0'; // Elimina el carácter de nueva línea
                        fflush(stdin);
                        printf("Ingrese los baudios: ");
                        scanf("%d", &baudios);
                        fflush(stdin);
                        //Mostramos por pantalla el puerto y los baudios seleccionados
                        printf("Puerto seleccionado: %s\n", com);
                        printf("Baudios: %d\n",baudios);
                        //Intentamos abrir el puerto
                        HANDLE port = openSerial(com, baudios);
                        //Control errores, no se abre el puerto si los baudios o el puerto introducidos son incorrectos
                        if (port != INVALID_HANDLE_VALUE) {
                            do{
                                system("cls");
                                printf("MENU STM32 \n\nIntrozuzca: \n (1) Enviarle LUCES y tiempo por puerto serie  (0) Para cerrar el puerto serie\n");
                                scanf("%c%c", &U, &buf);
                                fflush(stdin);
                                if ((U <= '1') && buf == '\n') {
                                    switch (U) {
                                        //Sera igual para los tres cases, solo que cambiando el parametro del PID que se trata
                                        case '1': {

                                            //Creamos un mensaje que es lo que queremos enviar
                                            uint8_t mensaje[200];
                                            //Un buff que sera el buffer del puerto serie por el que enviamos y recibimos
                                            uint8_t buff[200] = {0};
                                            //Control errores, valor antes de sumar 1

                                            //Primero le mandamos el numero de secuencias
                                            //Pasamos el numero de float a char
                                            snprintf(mensaje, sizeof(mensaje), "%hhu",L.Numero_estados_luces); // Convertir el número de nuevo a cadena
                                            //Le ponemos al final un \n para saber que es el final de la cadena de texto
                                            strcat(mensaje, "\n");
                                            //Mostramos en pantalla el valor que le vamos a enviar al STM32
                                            printf("Valor enviado al STM32: %s", mensaje);
                                            system("pause");
                                            //Escribimos el mensaje en el buffer para enviar
                                            write_port(port, (uint8_t *) mensaje, strlen(mensaje));
                                            FlushFileBuffers(port);
                                            Sleep(500); // Esperar un momento para asegurarse de que el mensaje ha sido procesado


                                            break;

                                        }
                                        case '0':
                                            CloseHandle(port);
                                            break;

                                        default:
                                            break;
                                    }
                                } else {
                                    puts("Introduce un numero valido");
                                    system("pause");
                                }
                            }while(U!='0');

                        }else {
                            printf("Error: No se pudo abrir el puerto serie\n");
                            system("pause");
                        }
                    }else{
                        //Mismo control de errores anteriormente explicado
                        printf("No hay ningun fichero de trabajo seleccionado. Desea crear uno nuevo por defecto?\n\n(1)Si\n(2)No\n");
                        char seleccion ;
                        scanf("%c",&seleccion);
                        fflush(stdin);
                        if(seleccion=='1'&& buf == '\n'){
                            archivotrabajo="NuevoDocumentoPID.txt";
                            newFile(archivotrabajo, L);
                            loadFile(archivotrabajo,L);
                        }else if(seleccion=='2'&& buf == '\n'){
                            break;
                        }else{
                            puts("Introduce un numero valido");
                            system("pause");
                        }

                    }

                }
                    break;

                case '0':
                    break;
            }
        }else{
            //Mismo control de errores anteriormente explicado
            puts("Introduce un numero valido");
            system("pause");
        }
    }while(x!='0');
    return 0;
}