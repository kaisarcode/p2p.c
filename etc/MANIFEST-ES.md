# Manifiesto HPM

## El ciberespacio como feria de barrio

### Una red no tiene por qué ser una plataforma

Durante años se volvió habitual pensar que para comunicarnos, publicar algo o participar de una comunidad primero debemos registrarnos en una plataforma.

- Crear una cuenta.
- Entregar un correo.
- Aceptar condiciones.
- Elegir una contraseña.
- Validar una identidad.
- Ser medidos.
- Ser clasificados.
- Ser recomendados o invisibilizados por un algoritmo.

HPM parte de otra idea:

> **No toda relación necesita una plataforma en el medio.**

Hay vínculos que ya existen antes de la tecnología: amistades, familias, barrios, cooperativas, comercios, clubes, grupos de trabajo, comunidades pequeñas. La red debería poder servir a esos vínculos sin apropiarse de ellos.

HPM no busca crear una comunidad de usuarios. Busca ofrecer herramientas para que comunidades reales puedan construir y compartir su propia red.

Por eso, este manifiesto habla de **red**, de **redes informáticas** y de **ciberespacio**. La web puede ser una parte de ese espacio, pero no lo agota. HPM conecta pares y servicios; HTTP y los sitios web son apenas una posibilidad entre muchas.

---

### No venimos a competir

HPM no pretende reemplazar Internet.

No pretende desbancar redes sociales, servicios de mensajería, proveedores de nube ni plataformas globales. Tampoco busca convertirse en la próxima red universal.

Nace porque existe una necesidad diferente.

Hay personas que no quieren abrir una cuenta nueva para cada cosa. Hay grupos que solo necesitan compartir un servidor, una página, una radio, un catálogo, un juego o una herramienta entre gente conocida. Hay comunidades que quieren usar sus propios dispositivos sin comprar un dominio, contratar hosting o depender de una empresa central.

HPM no sostiene que todas las redes deban funcionar de esta manera.

Sostiene algo más simple:

> **Esta forma de red también debe poder existir.**

---

### La feria de barrio

La mejor imagen para entender esta propuesta es una feria.

Uno no necesita crear una cuenta para pasar por una feria de barrio. Simplemente llega, recorre y mira qué hay.

- Algunos puestos están abiertos.
- Otros hoy no vinieron.
- Algunos aparecerán mañana.
- Una persona vende algo.
- Otra muestra su trabajo.
- Otra pone música.
- Otra invita a jugar.
- Otra comparte información.

Nadie promete estar disponible las veinticuatro horas. Lo que está, está porque alguien decidió sostenerlo en ese momento.

La red que imaginamos se parece a eso.

Un servicio puede aparecer y desaparecer. Puede existir solo por una tarde, durante una reunión, los fines de semana o mientras una computadora permanece encendida. Esa temporalidad no es necesariamente un defecto. Es parte de una infraestructura humana, cercana y honesta.

> **El ciberespacio como feria de barrio: abierto, cambiante, directo y sostenido por quienes participan.**

---

### Publisher, index y consumer

HPM ofrece un sistema mínimo de tres roles:

- un **publisher** publica un servicio;
- un **index** ayuda a encontrarlo;
- un **consumer** se conecta.

El índice no es una plataforma central, no aloja las aplicaciones y no transporta normalmente su tráfico. Actúa como un punto de encuentro: registra nombres, presenta extremos y ayuda a que dos dispositivos intenten comunicarse directamente.

Después, se aparta.

Los roles no pertenecen a clases especiales de infraestructura.

- Cualquiera puede tener un índice.
- Cualquiera puede publicar.
- Cualquiera puede consumir.
- Un mismo dispositivo puede cumplir más de un rol.

No existe un índice oficial obligatorio ni una red única a la cual haya que pertenecer.

---

### P2P significa P2P

La comunicación directa no es una optimización secundaria. Es uno de los principios del proyecto.

HPM fue diseñado para establecer caminos P2P entre los participantes. El índice coordina, pero no se convierte en el transportista permanente de la comunicación.

Por ese motivo, TURN queda fuera del núcleo del proyecto.

**TURN es un sistema de intermediación.** Cuando dos dispositivos no pueden conectarse directamente, ambos envían sus datos a un tercer servidor, que los recibe y los reenvía. Esto puede permitir la comunicación en redes restrictivas, pero también significa que el tráfico deja de viajar directamente entre los pares y pasa a depender de una infraestructura que permanece en medio de la conversación.

TURN puede ser útil en otros sistemas y una aplicación derivada puede implementarlo si lo necesita. Pero incorporar relays de tráfico como responsabilidad central de HPM cambiaría su naturaleza.

Un relay debe absorber ancho de banda, mantener sesiones, escalar recursos y convertirse en un punto privilegiado de observación y dependencia. Con suficiente crecimiento, ese modelo favorece infraestructuras centrales que solo actores con grandes recursos pueden sostener.

HPM elige otro límite:

> **Cuando una conexión directa no es posible, el núcleo puede fallar antes que dejar de ser P2P.**

No es una omisión accidental. Es una frontera ética y arquitectónica.

---

### Edge computing para dispositivos comunes

El índice no está pensado como un gran servidor central.

Debe poder funcionar en una computadora doméstica, una máquina de un comercio, un pequeño servidor, una placa de bajo consumo o cualquier dispositivo razonable disponible para la comunidad.

El trabajo se distribuye entre los extremos:

- los peers procesan sus datos;
- los peers sostienen sus servicios;
- HPM cifra por defecto las sesiones de su capa de transporte confiable entre pares;
- los peers aportan su ancho de banda;
- el índice realiza solo la coordinación necesaria.

Esto permite que la red crezca sin exigir que un nodo se transforme en una empresa de infraestructura.

> **La capacidad de la red surge de la suma de sus participantes, no del crecimiento de un centro.**

---

### Escalar no significa centralizar

HPM no escala creando un índice cada vez más grande.

Escala mediante muchos índices pequeños, independientes y superpuestos.

- Un publisher puede registrarse en varios índices.
- Un consumer puede consultar varios índices.
- Una aplicación puede mantener su propia lista de comunidades conocidas.

Un servicio puede estar presente en el índice de una familia, de un grupo de amigos, de una cooperativa y de un negocio local. Cada comunidad decide qué índices utiliza y cuáles comparte.

Los índices no tienen por qué sincronizarse entre sí ni obedecer a una autoridad global.

El crecimiento ocurre por tejido:

> **Más comunidades, más índices y más vínculos; no un servidor más poderoso y una autoridad más grande.**

---

### Redes de boca en boca

No toda red necesita descubrimiento global.

La dirección de un índice puede compartirse en una conversación, en un papel, mediante un código QR, en una memoria USB, en un archivo o por cualquier medio que la comunidad considere adecuado.

La entrada a la red puede comenzar con una frase sencilla:

> “Usá este índice y buscá este nombre.”

La confianza no tiene que surgir de una empresa que certifica a todos los participantes. Puede partir de relaciones humanas preexistentes.

Esto favorece redes pequeñas, comprensibles y apropiables. Redes cuyo alcance no depende de hacerse visibles ante todo el planeta.

---

### Sin cuentas por defecto

HPM no pide correo electrónico.

- No crea perfiles.
- No exige OAuth.
- No administra recuperación de cuentas.
- No construye una identidad global.
- No necesita saber quién es una persona.

Un identificador publicado en un índice no es una cuenta. Es un nombre operativo que permite encontrar un servicio en un contexto específico.

La participación no depende de pertenecer a HPM porque HPM no es una plataforma a la cual pertenecer.

> **Usar una herramienta no debería obligar a convertirse en usuario permanente de una empresa.**

---

### Passwords sin sistema de usuarios

Las contraseñas de HPM tienen un propósito particular.

`HPM_PASS` permite controlar quién puede publicar en un índice y reducir abusos.

`HPM_VIP` permite reservar IDs específicos para evitar que otra persona ocupe un nombre durante una caída o desconexión del publisher legítimo.

Esto resulta útil, por ejemplo, cuando un negocio publica servicios como:

- catálogo
- pedidos
- radio
- reservas

Si uno de esos servicios se desconecta, el seat puede permanecer protegido para que otro publisher no suplante temporalmente ese nombre.

Estas contraseñas no crean cuentas. No autentican consumidores. No definen identidades personales. No convierten el índice en un proveedor de acceso.

Protegen un recurso pequeño y concreto: la posibilidad de publicar y ocupar ciertos nombres.

---

### Todos los puestos son visibles

Las plataformas actuales suelen organizar la visibilidad mediante métricas, recomendaciones, publicidad y algoritmos de retención.

Un contenido aparece porque una empresa decidió mostrarlo. Otro desaparece porque no produce interacción suficiente. Las relaciones se traducen en seguidores, likes, alcance y rendimiento.

La feria propone algo distinto.

Los puestos están ahí. Uno los recorre y los encuentra. No necesitan competir por atención ante una máquina que decide quién merece ser visto.

Una aplicación construida sobre HPM podría mostrar servicios con:

- nombre;
- ícono;
- descripción;
- categoría;
- estado;
- información comunitaria.

Podría parecerse visualmente a un directorio de servidores, una tienda de aplicaciones o una cartelera. Pero no tendría por qué convertirse en un mercado de visibilidad.

> **Estar presente debería ser suficiente para poder ser encontrado.**

---

### La comunidad no es una métrica

En muchas plataformas, “comunidad” significa una masa de usuarios reunida alrededor de una infraestructura que no controla.

- La plataforma define las reglas.
- La plataforma conserva los datos.
- La plataforma decide la visibilidad.
- La plataforma puede cambiar las condiciones.
- La plataforma puede cerrar el espacio.

Eso no siempre constituye una comunidad autónoma. Con frecuencia es una audiencia alquilada.

Como consecuencia, quienes publican terminan condicionados por los intereses de la plataforma. Ya no alcanza con crear o compartir lo que consideran valioso: deben adaptarse a aquello que produce retención, interacción o rentabilidad. Cuando no lo hacen, el propio sistema puede relegar, ocultar o volver irrelevante su trabajo.

HPM parte de una noción diferente:

> **Una comunidad es un grupo capaz de sostener sus propios vínculos y sus propios espacios.**

La tecnología debe facilitar esa autonomía, no sustituirla.

---

### Recuperar una posibilidad del ciberespacio

Conviene recuperar una palabra que hoy parece antigua: **ciberespacio**.

El término permite pensar la red como un ámbito compartido, compuesto por lugares, vínculos, servicios y comunidades, y no solamente como una colección de sitios web o aplicaciones comerciales. HPM no conecta únicamente páginas: conecta pares capaces de ofrecer juegos, radios, archivos, herramientas, catálogos, servicios interactivos o cualquier otra aplicación que pueda construirse sobre una red informática.

La Web temprana fue una de las expresiones más visibles de esa idea. Estaba formada por lugares: páginas personales, foros, servidores, directorios y espacios construidos por personas u organizaciones concretas. No todo ocurría dentro de unas pocas plataformas.

Pero publicar también era difícil. Requería conocimientos, infraestructura y recursos que no estaban al alcance de todos. Durante mucho tiempo, las redes fueron territorio de universidades, grandes empresas y especialistas.

Después llegó la masificación, pero también la concentración.

Hoy la situación material ha cambiado. Millones de personas llevan en el bolsillo computadoras que habrían sido inimaginables décadas atrás. Los hogares y pequeños negocios cuentan con dispositivos capaces de publicar, procesar y almacenar información.

Es posible volver a pensar redes descentralizadas desde otras condiciones.

No para recrear el pasado exactamente, sino para recuperar una de sus posibilidades:

> **Que cualquiera pueda tener un lugar propio en la red.**

Existen muchas otras propuestas P2P, cada una con objetivos y decisiones propias. Algunas se orientan principalmente a la transferencia de archivos; otras nacieron ligadas a tecnologías web, al anonimato, a la mensajería o a redes especializadas. HPM no pretende reemplazarlas ni competir con ellas.

Su propuesta es más acotada: ofrecer una base sencilla y de uso general para que una comunidad pueda conectar pares y construir encima aquello que necesite. No define de antemano si lo compartido será una página, una radio, un juego, un catálogo o un servicio todavía no imaginado.

---

### Publicar sin comprar presencia

Una persona puede tener un servidor de Minecraft en su casa y compartirlo con amigos sin contratar un hosting ni comprar un dominio.

Un comercio puede publicar un catálogo desde la computadora del local.

Una familia puede compartir archivos o herramientas.

Un grupo puede levantar una radio.

Una cooperativa puede ofrecer servicios internos.

Un barrio puede tener una cartelera o una pequeña web comunitaria.

La infraestructura sigue perteneciendo a quienes la usan. No hace falta comprar presencia digital a un intermediario para poder existir ante los demás.

---

### Las limitaciones también son diseño

HPM es una biblioteca pequeña y deliberadamente acotada.

No incluye:

- identidad global;
- perfiles;
- seguidores;
- likes;
- rankings;
- telemetría obligatoria;
- moderación universal;
- almacenamiento central;
- sincronización mundial de índices;
- disponibilidad permanente;
- relays obligatorios;
- una autoridad oficial.

Estas ausencias no deben interpretarse automáticamente como errores o funcionalidades pendientes.

Algunas capacidades, al incorporarse al núcleo, crean centros de control, necesidades de infraestructura y posibilidades de concentración.

Las limitaciones pueden proteger una escala.

> **No toda herramienta debe diseñarse para convertirse en monopolio.**

---

### Una base, no una aplicación total

HPM resuelve el sistema mínimo de publisher, index y consumer.

Las capas superiores pueden agregar:

- listas de índices;
- catálogos visuales;
- metadatos;
- descripciones;
- íconos;
- protocolos comunitarios;
- permisos;
- identidad local;
- sistemas de invitación;
- formas externas de relay;
- experiencias específicas para juegos, radios, páginas o comercios.

Cada aplicación decide cuánto construir y bajo qué valores.

El núcleo no intenta anticipar todas las posibilidades ni imponer una única forma de comunidad.

> **HPM entrega herramientas; las comunidades construyen sus espacios.**

---

### Libertad para crear y libertad para irse

Una infraestructura comunitaria debe permitir que sus participantes:

- levanten su propio índice;
- cambien de índice;
- publiquen en varios;
- dejen de publicar;
- creen otra red;
- compartan herramientas;
- modifiquen el software;
- no dependan de una instancia oficial.

Ningún nodo debe ser irremplazable.

Ninguna comunidad debe quedar atrapada porque una empresa posee su identidad, su historial o su acceso.

La libertad no consiste solamente en poder entrar. También consiste en poder salir, copiar, reemplazar y volver a empezar.

---

### No buscamos usuarios cautivos

HPM no necesita crecer indefinidamente para justificar su existencia.

- Una red utilizada por cinco amigos puede ser exitosa.
- Una red familiar puede ser suficiente.
- Una instalación para un comercio puede cumplir su propósito.
- Una comunidad pequeña no es una etapa incompleta hacia una plataforma masiva.

El valor no se mide por cantidad de cuentas, tiempo de permanencia o cuota de mercado.

> **Una herramienta puede ser importante sin conquistar el mundo.**

---

### Nuestra propuesta

Esta no es la única forma posible de construir redes descentralizadas.

No pretende ser la respuesta definitiva ni universal. Existen muchos proyectos, protocolos y comunidades que exploran caminos distintos.

HPM es una propuesta concreta:

- comunicación P2P;
- infraestructura mínima;
- índices pequeños;
- participación sin cuentas;
- publicación desde dispositivos comunes;
- redes efímeras;
- descubrimiento de boca en boca;
- aplicaciones construidas por encima;
- comunidades antes que plataformas;
- límites deliberados contra la concentración.

No afirmamos que toda red deba ser una feria.

Afirmamos que también debe haber lugar para las ferias.

---

## Principios

### La red debe servir a la comunidad

La comunidad no debe convertirse en materia prima de una plataforma.

### Publicar no debe requerir permiso

Quien posee un dispositivo y una conexión debe poder compartir un servicio.

### Participar no debe requerir una cuenta

No toda interacción necesita identidad global, correo, contraseña u OAuth.

### El índice coordina, no gobierna

Ayuda a que los peers se encuentren y luego se aparta de la comunicación.

### La ruta de datos debe ser directa

El P2P es un principio del núcleo, no una opción cosmética.

### La escala debe surgir de la multiplicación

Muchos índices pequeños son preferibles a un centro obligatorio.

### La temporalidad es legítima

Un servicio puede estar hoy y no mañana. Eso también es red.

### La visibilidad no debe depender de métricas

Los puestos existen para ser recorridos, no para competir ante un algoritmo.

### Las limitaciones pueden proteger la libertad

No toda capacidad técnica es deseable dentro del núcleo.

### El ciberespacio no es solamente la web

Las páginas y los sitios son una posibilidad, no el límite. La red también puede transportar juegos, radios, archivos, herramientas, catálogos, conversaciones y servicios todavía no imaginados.

### Nadie debe poseer la red completa

Cada persona y cada comunidad deben poder levantar, modificar y reemplazar sus propias piezas.

---

## Cierre

HPM no quiere construir una nueva plataforma central.

Quiere hacer más corto el camino entre una persona que tiene algo para compartir y una comunidad que quiere encontrarlo.

- Sin cuentas innecesarias.
- Sin dominios obligatorios.
- Sin hosting como requisito.
- Sin telemetría inherente.
- Sin algoritmos de visibilidad.
- Sin una autoridad central indispensable.

- Una persona abre un puesto.
- Otra pasa y mira.
- Alguien recomienda algo.
- Un servicio aparece.
- Otro se apaga.
- La comunidad cambia.

Nada promete ser global. Nada necesita ser eterno. Nada tiene que convertirse en monopolio para tener valor.

> **HPM propone la red como un espacio común: directo, pequeño, libre y profundamente humano.**
