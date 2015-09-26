#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include "string.h"
#include <unistd.h>


const char delimitador[]="<end-document>"; // 14 caracteres
const char espacio[] = " ";
const int MAX_LINEA= 200;
const int MAX_PALABRA= 20;
FILE* stopwords;
char documento_stopwords[200];
int repeticion(char *palabra, char ** palabras_documento, int cantidad_palabras);

int main (int argc, char *argv[]){
   int rank, size, get, cantidad_lineas=0,diferencia,cantidad_documentos=0,i,k;
   char *linea,**total_lineas,**Documentos;
   FILE *archivo;
   MPI_Init (&argc, &argv);	                /* Inicia MPI */
   MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* obtiene el id del proceso actual */
   MPI_Comm_size (MPI_COMM_WORLD, &size);	/* obtiene el total de procesos */
// SS1 ------------------------------------------------------------------------------
// Leo todas las lineas del documento y las envio a los procesos equitativamente para que creen el vocabulario
   if(rank==0){
      while((get=getopt(argc,argv,"f:s:"))!=-1)
      switch(get){
         case 'f':
            archivo = fopen(optarg,"r");
            if(archivo==NULL){
               printf("No se puede abrir el fichero %s\n",optarg);
               return 0;         
            }        
            break;
         case 's':
            stopwords = fopen(optarg,"r");
            if(archivo==NULL){
               printf("No se puede abrir el fichero de stopwords %s\n",optarg);
               return 0;         
            }
            fclose(stopwords);        
            break;
      }
      linea = (char *) malloc(sizeof(char)*MAX_LINEA);
      while(!feof(archivo)){
         if(fgets(linea,MAX_LINEA,archivo)==NULL){
            break;
         }
         cantidad_lineas ++;
         diferencia = strncmp(delimitador,linea,14); // verifico si la linea leida es el final del documento 
         if(diferencia==0){
            cantidad_documentos++;
         }
      }
      cantidad_lineas -=cantidad_documentos;
      free(linea);
      total_lineas = (char **) malloc(sizeof(char *)*cantidad_lineas);
      for(i=0;i<cantidad_lineas;i++){
         total_lineas[i]= (char *) malloc(sizeof(char)*MAX_LINEA);
      }
      rewind(archivo);
      linea = (char *) malloc(sizeof(char)*MAX_LINEA);
      i = 0;
      while(!feof(archivo)){ // obtengo las lineas del archivo completo y las paso a una matriz de char 
         if(fgets(linea,MAX_LINEA,archivo)==NULL){
            break;
         }
         diferencia = strncmp(delimitador,linea,14); // verifico si la linea leida es el final del documento 
         if(diferencia!=0){
            strcpy(total_lineas[i],linea);
            i++;
         }
      }
      int seguir = 1;
      rewind(archivo);
      int *cantidad_lineas_por_doc = (int *)malloc(sizeof(int)*cantidad_documentos);
      for (i = 0; i < cantidad_documentos; ++i){ // obtengo la cantidad de lineas por documento, del archivo. en un documento 
         cantidad_lineas_por_doc[i]=0;
         while(seguir==1){ 
            if(fgets(linea,MAX_LINEA,archivo)==NULL){
               break;
            }
            diferencia = strncmp(delimitador,linea,14); // verifico si la linea leida es el final del documento 
            if(diferencia!=0){
               cantidad_lineas_por_doc[i]++;
            }else{
               seguir=0;
            }
         }
         seguir=1;
      }
      rewind(archivo);

      Documentos= (char **)malloc(sizeof(char *)*cantidad_documentos);
      for (i = 0; i < cantidad_documentos; ++i){
         Documentos[i]= (char *)malloc(sizeof(char)*(cantidad_lineas_por_doc[i]*MAX_LINEA));
      }

      /*for (i = 0; i < cantidad_documentos; ++i){
         printf("Documento %i : Lineas -> %i\n",i,cantidad_lineas_por_doc[i] );
      }*/
      int indice=0;
      for (i = 0; i < cantidad_documentos; ++i){ // obtengo las lineas correspondientes a cada documento y las concateno
         strcpy(Documentos[i],total_lineas[indice]);
         indice++;
         for(k=1;k<cantidad_lineas_por_doc[i];k++){
            strcat(Documentos[i],total_lineas[indice]);
            indice++;
         }
      }
      int tamano_envio=0;
      for (i = 0; i < cantidad_documentos; ++i){
         tamano_envio = cantidad_lineas_por_doc[i]*MAX_LINEA;
         //MPI_Send(Documentos[i],cantidad_lineas_por_doc[i]*MAX_LINEA,MPI_CHAR,i+1,cantidad_lineas_por_doc[i]*MAX_LINEA,MPI_COMM_WORLD);
         MPI_Send(&tamano_envio,1,MPI_INT,(i+1)%size,0,MPI_COMM_WORLD); // envio tamano del documento
         MPI_Send(Documentos[i],cantidad_lineas_por_doc[i]*MAX_LINEA,MPI_CHAR,(i+1)%size,1,MPI_COMM_WORLD);
      }

      free(Documentos);
      free(linea);
      free(total_lineas);

  }
   MPI_Barrier(MPI_COMM_WORLD);
// SS2 -------------------------------------------------------------------------------------------------
// Obtengo las palabras del documento y creo el vocabulario local

   if(rank!=0){ // comienzo a recivir mi documento asignado

      int tamano,seguir =1,cantidad_palabras=0,*repeticion_palabras;
      char *documento_local, **palabras_del_documento, *documento_local_temp,**vocabulario;
      char car_inutiles[4]=" \n\t";
      char *palabra;

      MPI_Status *estado = (MPI_Status*)malloc(sizeof(MPI_Status));
      MPI_Recv(&tamano,1,MPI_INT,0,0,MPI_COMM_WORLD,estado);
      documento_local = (char *)malloc(sizeof(char)*tamano);
      documento_local_temp = (char *)malloc(sizeof(char)*tamano);
      MPI_Recv(documento_local,tamano,MPI_CHAR,0,1,MPI_COMM_WORLD,estado);
      strcpy(documento_local_temp,documento_local);

      palabra = strtok( documento_local, car_inutiles );    // Primera llamada => Primer token
      cantidad_palabras++;
      while( (palabra = strtok( NULL, car_inutiles )) != NULL )    // Posteriores llamadas
        {cantidad_palabras++;}
      printf("Proceso [ %i ] Con Paalabras: %i\n",rank,cantidad_palabras );

      palabras_del_documento = (char **)malloc(sizeof(char *)*cantidad_palabras);
      for (i = 0; i < cantidad_palabras; ++i)
      {
         palabras_del_documento[i]=(char*)malloc(sizeof(char)*MAX_PALABRA);
      }

      int k = 0;
      palabra = strtok( documento_local_temp, car_inutiles );    // Primera llamada => Primer token
      strcpy(palabras_del_documento[k],palabra);
      palabra=NULL;
      k++;      
      while( (palabra = strtok( NULL, car_inutiles )) != NULL )    // Posteriores llamadas
        {
         strcpy(palabras_del_documento[k],palabra);
         k++;
      }

      repeticion_palabras = (int *)malloc(sizeof(int)*cantidad_palabras);
      int cantidad=0,j;
      for (i = 0; i < cantidad_palabras; ++i)
      {
         cantidad = repeticion(palabras_del_documento[i],palabras_del_documento,cantidad_palabras);
         repeticion_palabras[i]=cantidad;
      }
       /*for (i = 0; i < cantidad_palabras; ++i)
      {
         printf("[%i] : %s - %i\n",rank,palabras_del_documento[i],repeticion_palabras[i] );
      }*/
      // quito las palabras repetidas y las agrego a mi vocabulario local
      int cantidad_palabras_no_repetidas=0;
      for(i=0;i<cantidad_palabras;i++){
         if(repeticion_palabras[i]!=0){
            cantidad_palabras_no_repetidas++;
            repeticion_palabras[i]=0;
            for (j = i; j < cantidad_palabras; j++)
            {
               if(strncmp(palabras_del_documento[i],palabras_del_documento[j],MAX_PALABRA)==0){
                  repeticion_palabras[j]=0;
               }
            }
         }
      }

      printf("%i - Cantidad Palabras no repetidas: %i \n",rank,cantidad_palabras_no_repetidas);
   }

   MPI_Finalize();                               /* Finaliza MPI */
   return 0;
}

int repeticion(char *palabra, char ** palabras_documento, int cantidad_palabras){
   int i,resultado=0,diferencia;
   for(i=0;i<cantidad_palabras;i++){
      diferencia=strncmp(palabra,palabras_documento[i],MAX_PALABRA);
      if(diferencia==0){
         resultado++;
      }
   }
   return resultado;
}
