#include "headers/game_state.h"
#include "headers/localization.h"
#include <fstream>
#include <iostream>
#include <sstream>

std::string currentHUDMessage = "";
float hudMessageTimer = 0.0f;
bool isReadingDocument = false;
std::string currentDocumentTitle = "";
std::string currentDocumentBody = "";

glm::vec3 mensBpos(35.231f, 0.250f, 9.150f);
glm::vec3 mensBrot(90.000f, 0.000f, 0.000f);
glm::vec3 mensBscale(1.700f, 1.700f, 1.700f);

glm::vec3 girlBpos(38.776f, 0.250f, 9.150f);
glm::vec3 girlBrot(90.000f, 0.000f, 0.000f);
glm::vec3 girlBscale(1.700f, 1.700f, 1.700f);

glm::vec3 mirrorBGpos(40.85f, 0.150f, 4.50f);
glm::vec3 mirrorBGRot(0.0f, -90.0f, 0.0f);
glm::vec3 mirrorBGScale(3.20f, 1.0f, 0.05f);

glm::vec3 azulejoPos(35.160f, 0.200f, 7.00f);
glm::vec3 azulejoRot(90.0f, -90.0f, 0.0f);
glm::vec3 azulejoScale(0.540f, 0.520f, 0.630f);

glm::vec3 mirrorPos(33.160f, 0.100f, 3.00f);
glm::vec3 mirrorRot(90.0f, -90.0f, 0.0f);
glm::vec3 mirrorScale(0.540f, 0.520f, 0.630f);

glm::vec3 mirrorPos2(33.160f, 0.100f, 4.00f);
glm::vec3 mirrorRot2(90.0f, -90.0f, 0.0f);
glm::vec3 mirrorScale2(0.540f, 0.520f, 0.630f);

glm::vec3 mirrorPos3(33.160f, 0.100f, 5.00f);
glm::vec3 mirrorRot3(90.0f, -90.0f, 0.0f);
glm::vec3 mirrorScale3(0.540f, 0.520f, 0.630f);

glm::vec3 mirrorPos4(33.160f, 0.1f, 6.00f);
glm::vec3 mirrorRot4(90.0f, -90.0f, 0.0f);
glm::vec3 mirrorScale4(0.540f, 0.520f, 0.630f);

glm::vec3 ligthbathroom2Pos(35.160f, 0.480f, 3.403f);
glm::vec3 ligthbathroom2Rot(0.0f, -180.0f, 0.0f);
glm::vec3 ligthbathroom2Scale(0.520f, 0.490f, 1.0f);
bool ligthbathroomDebugVisible2 = true;

glm::vec3 lamp3Pos(38.160f, 0.480f, 3.403f);
glm::vec3 lamp3Rot(0.0f, -180.0f, 0.0f);
glm::vec3 lamp3Scale(0.520f, 0.490f, 1.0f);

glm::vec3 lamp4Pos(38.160f, 0.480f, 7.403f);
glm::vec3 lamp4Rot(0.0f, -180.0f, 0.0f);
glm::vec3 lamp4Scale(0.520f, 0.490f, 1.0f);

glm::vec3 ligthbathroomPos(35.160f, 0.480f, 7.403f);
glm::vec3 ligthbathroomRot(0.0f, -180.0f, 0.0f);
glm::vec3 ligthbathroomScale(0.520f, 0.490f, 1.0f);
bool ligthbathroomDebugVisible = true;

glm::vec3 banoPos(35.6f, -0.5f, 1.740f);
glm::vec3 banoRot(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos2(36.150f, -0.5f, 1.740f);
glm::vec3 banoRot2(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale2(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos3(35.050f, -0.5f, 1.740f);
glm::vec3 banoRot3(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale3(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos4(34.500f, -0.5f, 1.740f);
glm::vec3 banoRot4(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale4(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos5(38.400f, -0.5f, 1.740f);
glm::vec3 banoRot5(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale5(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos6(37.850f, -0.5f, 1.740f);
glm::vec3 banoRot6(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale6(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos7(38.950f, -0.5f, 1.740f);
glm::vec3 banoRot7(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale7(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos8(39.500f, -0.5f, 1.740f);
glm::vec3 banoRot8(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale8(0.5f, 0.4f, 0.4f);

glm::vec3 lavamanosPos(33.250f, -0.300f, 3.00f);
glm::vec3 lavamanosRot(0.0f, -90.0f, 0.0f);
glm::vec3 lavamanosScale(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos2(33.250f, -0.300f, 4.00f);
glm::vec3 lavamanosRot2(0.0f, -90.0f, 0.0f);
glm::vec3 lavamanosScale2(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos3(33.250f, -0.300f, 5.00f);
glm::vec3 lavamanosRot3(0.0f, -90.0f, 0.0f);
glm::vec3 lavamanosScale3(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos4(33.250f, -0.300f, 6.00f);
glm::vec3 lavamanosRot4(0.0f, -90.0f, 0.0f);
glm::vec3 lavamanosScale4(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos5(40.750f, -0.300f, 3.00f);
glm::vec3 lavamanosRot5(0.0f, 90.0f, 0.0f);
glm::vec3 lavamanosScale5(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos6(40.750f, -0.300f, 4.00f);
glm::vec3 lavamanosRot6(0.0f, 90.0f, 0.0f);
glm::vec3 lavamanosScale6(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos7(40.750f, -0.300f, 5.00f);
glm::vec3 lavamanosRot7(0.0f, 90.0f, 0.0f);
glm::vec3 lavamanosScale7(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos8(40.750f, -0.300f, 6.00f);
glm::vec3 lavamanosRot8(0.0f, 90.0f, 0.0f);
glm::vec3 lavamanosScale8(0.540f, 0.520f, 0.630f);

glm::vec3 urinarioPos(36.8f, -0.54f, 5.2f);
glm::vec3 urinarioRot(-90.5f, -2.5f, -90.0f);
glm::vec3 urinarioScale(0.4f, 0.4f, 0.4f);

glm::vec3 sillasPos = glm::vec3(30.3f, -0.35f, 8.6f);
glm::vec3 sillasRot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 sillasScale = glm::vec3(0.65f, 0.65f, 0.65f);

glm::vec3 sillas2Pos = glm::vec3(26.7f, -0.35f, 8.6f);
glm::vec3 sillas2Rot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 sillas2Scale = glm::vec3(0.65f, 0.65f, 0.65f);

glm::vec3 sillas3Pos = glm::vec3(31.8f, -0.35f, 8.6f);
glm::vec3 sillas3Rot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 sillas3Scale = glm::vec3(0.65f, 0.65f, 0.65f);

glm::vec3 sillas4Pos = glm::vec3(24.0f, -0.35f, 8.6f);
glm::vec3 sillas4Rot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 sillas4Scale = glm::vec3(0.65f, 0.65f, 0.65f);

glm::vec3 sillas5Pos = glm::vec3(21.3f, -0.35f, 8.6f);
glm::vec3 sillas5Rot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 sillas5Scale = glm::vec3(0.65f, 0.65f, 0.65f);

glm::vec3 sofaPos = glm::vec3(13.7f, -0.40f, 1.7f);
glm::vec3 sofaRot = glm::vec3(90.0f, 180.0f, 0.0f);
glm::vec3 sofaScale = glm::vec3(0.5f, 0.5f, 0.5f);

glm::vec3 monitorPos = glm::vec3(32.75f, 0.0f, 5.2f);
glm::vec3 monitorRot = glm::vec3(0.0f, -180.0f, 0.0f);
glm::vec3 monitorScale = glm::vec3(1.5f, 1.5f, 1.5f);

glm::vec3 monitor2Pos = glm::vec3(13.4f, 0.0f, 5.2f);
glm::vec3 monitor2Rot = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 monitor2Scale = glm::vec3(1.5f, 1.5f, 1.5f);

glm::vec3 maquinaPos = glm::vec3(30.38f, 0.0f, 7.8f);
glm::vec3 maquinaRot = glm::vec3(0.0f, 0.0f, 180.0f);
glm::vec3 maquinaScale = glm::vec3(0.5f, 0.5f, 0.5f);

glm::vec3 deskPos = glm::vec3(15.600f, -0.300f, 3.000f);
glm::vec3 deskRot = glm::vec3(0.0f, -90.0f, 13.0f);
glm::vec3 deskScale = glm::vec3(0.60f, 0.60f, 0.60f);

glm::vec3 desk2Pos = glm::vec3(17.600f, -0.300f, 3.000f);
glm::vec3 desk2Rot = glm::vec3(0.0f, -90.0f, 13.0f);
glm::vec3 desk2Scale = glm::vec3(0.600f, 0.600f, 0.600f);

glm::vec3 desk3Pos = glm::vec3(15.600f, -0.300f, 5.300f);
glm::vec3 desk3Rot = glm::vec3(0.0f, -90.0f, 13.0f);
glm::vec3 desk3Scale = glm::vec3(0.600f, 0.600f, 0.600f);

glm::vec3 desk4Pos = glm::vec3(17.400f, -0.300f, 5.300f);
glm::vec3 desk4Rot = glm::vec3(0.0f, -90.0f, 13.0f);
glm::vec3 desk4Scale = glm::vec3(0.600f, 0.600f, 0.600f);

glm::vec3 desk5Pos = glm::vec3(19.400f, -0.300f, 5.200f);
glm::vec3 desk5Rot = glm::vec3(0.0f, -90.0f, 13.0f);
glm::vec3 desk5Scale = glm::vec3(0.600f, 0.600f, 0.600f);

glm::vec3 desk6Pos = glm::vec3(19.400f, -0.300f, 3.000f);
glm::vec3 desk6Rot = glm::vec3(0.0f, -90.0f, 13.0f);
glm::vec3 desk6Scale = glm::vec3(0.600f, 0.600f, 0.600f);

glm::vec3 sillita1Pos = glm::vec3(15.600f, -0.300f, 2.500f);
glm::vec3 sillita1Rot = glm::vec3(0.0f, -90.0f, 0.0f);
glm::vec3 sillita1Scale = glm::vec3(0.600f, 0.600f, 0.600f);

glm::vec3 sillita2Pos = glm::vec3(17.600f, -0.300f, 2.500f);
glm::vec3 sillita2Rot = glm::vec3(0.0f, -90.0f, 0.0f);
glm::vec3 sillita2Scale = glm::vec3(0.600f, 0.600f, 0.600f);

glm::vec3 sillita3Pos = glm::vec3(15.600f, -0.300f, 4.800f);
glm::vec3 sillita3Rot = glm::vec3(0.0f, -90.0f, 0.0f);
glm::vec3 sillita3Scale = glm::vec3(0.600f, 0.600f, 0.600f);

glm::vec3 sillita4Pos = glm::vec3(17.400f, -0.300f, 4.800f);
glm::vec3 sillita4Rot = glm::vec3(0.0f, -90.0f, 0.0f);
glm::vec3 sillita4Scale = glm::vec3(0.600f, 0.600f, 0.600f);

glm::vec3 sillita5Pos = glm::vec3(19.400f, -0.300f, 2.500f);
glm::vec3 sillita5Rot = glm::vec3(0.0f, -90.0f, 0.0f);
glm::vec3 sillita5Scale = glm::vec3(0.600f, 0.600f, 0.600f);

glm::vec3 sillita6Pos = glm::vec3(19.400f, -0.300f, 4.800f);
glm::vec3 sillita6Rot = glm::vec3(0.0f, -90.0f, 0.0f);
glm::vec3 sillita6Scale = glm::vec3(0.600f, 0.600f, 0.600f);

glm::vec3 estantePos = glm::vec3(15.4f, -0.35f, 1.7f);
glm::vec3 estanteRot = glm::vec3(0.0f, -90.0f, 0.0f);
glm::vec3 estanteScale = glm::vec3(0.8f, 0.8f, 0.8f);

glm::vec3 estante2Pos = glm::vec3(17.4f, -0.35f, 1.7f);
glm::vec3 estante2Rot = glm::vec3(0.0f, -90.0f, 0.0f);
glm::vec3 estante2Scale = glm::vec3(0.8f, 0.8f, 0.8f);

glm::vec3 estante3Pos = glm::vec3(19.5f, -0.35f, 1.7f);
glm::vec3 estante3Rot = glm::vec3(0.0f, -90.0f, 0.0f);
glm::vec3 estante3Scale = glm::vec3(0.8f, 0.8f, 0.8f);

glm::vec3 lockerPos = glm::vec3(25.3f, -0.35f, 8.5f);
glm::vec3 lockerRot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 lockerScale = glm::vec3(0.8f, 0.8f, 0.8f);

glm::vec3 locker2Pos = glm::vec3(22.60f, -0.35f, 8.5f);
glm::vec3 locker2Rot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 locker2Scale = glm::vec3(0.8f, 0.8f, 0.8f);

glm::vec3 locker3Pos = glm::vec3(30.0f, -0.35f, 1.8f);
glm::vec3 locker3Rot = glm::vec3(0.0f, -90.0f, 0.0f);
glm::vec3 locker3Scale = glm::vec3(0.8f, 0.8f, 0.8f);

glm::vec3 locker4Pos = glm::vec3(27.4f, -0.35f, 1.8f);
glm::vec3 locker4Rot = glm::vec3(0.0f, -90.0f, 0.0f);
glm::vec3 locker4Scale = glm::vec3(0.8f, 0.8f, 0.8f);

glm::vec3 estantesPos = glm::vec3(22.6f, 0.0f, 12.8f);
glm::vec3 estantesRot = glm::vec3(0.0f, 180.0f, 0.0f);
glm::vec3 estantesScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 morguefridgePos = glm::vec3(22.6f, 0.0f, 16.0f);
glm::vec3 morguefridgeRot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 morguefridgeScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 monitoringPos = glm::vec3(20.3f, -0.35f, 15.1f);
glm::vec3 monitoringRot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 monitoringScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 refrigeradorPos = glm::vec3(22.6f, -0.35f, 12.9f);
glm::vec3 refrigeradorRot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 refrigeradorScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 camillaPos = glm::vec3(19.1f, -0.35f, 18.8f);
glm::vec3 camillaRot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 camillaScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 muralPos = glm::vec3(7.4f, 0.5f, 14.1f);
glm::vec3 muralRot = glm::vec3(0.0f, 180.0f, 0.0f);
glm::vec3 muralScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 terminalesPos = glm::vec3(8.0f, -0.35f, 13.0f);
glm::vec3 terminalesRot = glm::vec3(0.0f, 180.0f, 0.0f);
glm::vec3 terminalesScale = glm::vec3(0.7f, 0.7f, 0.7f);

glm::vec3 esferaPos = glm::vec3(10.6f, -0.35f, 20.0f);
glm::vec3 esferaRot = glm::vec3(0.0f, 180.0f, 0.0f);
glm::vec3 esferaScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 bodybagPos = glm::vec3(7.8f, -0.35f, 14.3f);
glm::vec3 bodybagRot = glm::vec3(0.0f, 180.0f, 0.0f);
glm::vec3 bodybagScale = glm::vec3(0.7f, 0.7f, 0.7f);

glm::vec3 coffinPos = glm::vec3(13.4f, -0.35f, 17.6f);
glm::vec3 coffinRot = glm::vec3(0.0f, 45.0f, 0.0f);
glm::vec3 coffinScale = glm::vec3(0.7f, 0.7f, 0.7f);

glm::vec3 bloodyboxPos = glm::vec3(8.5f, -0.35f, 16.4f);
glm::vec3 bloodyboxRot = glm::vec3(0.0f, 180.0f, 0.0f);
glm::vec3 bloodyboxScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 labtablePos = glm::vec3(12.0f, -0.35f, 13.0f);
glm::vec3 labtableRot = glm::vec3(0.0f, 180.0f, 0.0f);
glm::vec3 labtableScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 shelfPos = glm::vec3(22.5f, -0.35f, 13.7f);
glm::vec3 shelfRot = glm::vec3(0.0f, 180.0f, 0.0f);
glm::vec3 shelfScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 safetyPos = glm::vec3(9.0f, -0.35f, 20.6f);
glm::vec3 safetyRot = glm::vec3(0.0f, 180.0f, 0.0f);
glm::vec3 safetyScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 neveraPos = glm::vec3(6.6f, -0.35f, 13.9f); 
glm::vec3 neveraRot = glm::vec3(0.0f, 90.0f, 0.0f);   
glm::vec3 neveraScale = glm::vec3(0.6f, 0.6f, 0.6f); 

glm::vec3 estantebodPos = glm::vec3(4.9f, -0.35f, 15.6f); 
glm::vec3 estantebodRot = glm::vec3(0.0f, 90.0f, 0.0f);   
glm::vec3 estantebodScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 boxesPos = glm::vec3(4.9f, -0.35f, 16.0f); 
glm::vec3 boxesRot = glm::vec3(0.0f, 90.0f, 0.0f);   
glm::vec3 boxesScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 barrilPos = glm::vec3(4.1f, -0.35f, 15.2f); 
glm::vec3 barrilRot = glm::vec3(0.0f, 180.0f, 0.0f);   
glm::vec3 barrilScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 paredCuarPos   = glm::vec3(33.1f, -0.35f, 20.6f); 
glm::vec3 paredCuarRot   = glm::vec3(0.0f, 180.0f, 0.0f);   
glm::vec3 paredCuarScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 capsulasPos   = glm::vec3(29.0f, -0.35f, 16.4f); 
glm::vec3 capsulasRot   = glm::vec3(0.0f, 180.0f, 0.0f);   
glm::vec3 capsulasScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 barrilesPos   = glm::vec3(23.4f, -0.35f, 20.2f); 
glm::vec3 barrilesRot   = glm::vec3(0.0f, 180.0f, 0.0f);   
glm::vec3 barrilesScale = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 bigtankPos     = glm::vec3(33.1f, -0.35f, 20.6f); 
glm::vec3 bigtankRot     = glm::vec3(0.0f, 180.0f, 0.0f);   
glm::vec3 bigtankScale   = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 generadoresPos     = glm::vec3(33.1f, -0.35f, 20.6f); 
glm::vec3 generadoresRot     = glm::vec3(0.0f, 180.0f, 0.0f);   
glm::vec3 generadoresScale   = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 subjectPos     = glm::vec3(26.7f, -0.35f, 12.8f); 
glm::vec3 subjectRot     = glm::vec3(0.0f, 180.0f, 0.0f);   
glm::vec3 subjectScale   = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 sciboxPos     = glm::vec3(29.9f, -0.35f, 16.4f); 
glm::vec3 sciboxRot     = glm::vec3(0.0f, 180.0f, 0.0f);   
glm::vec3 sciboxScale   = glm::vec3(0.6f, 0.6f, 0.6f);

glm::vec3 cameraPos = glm::vec3(6.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float headBobTimer = 0.0f;
float baseCameraY = 0.0f;
bool isMoving = false;
float stamina = 100.0f;
bool isSprinting = false;
bool isExhausted = false;
GameState gameState = MENU;
bool menuOpcionesActivo = false;
bool menuControlesActivo = false;
bool menuCreditosActivo = false;
bool juegoMuteado = false;
bool isCursorLocked = false;
bool tabKeyWasPressed = false;
bool eKeyWasPressed = false;
bool zKeyWasPressed = false;
bool interactionPressedThisFrame = false;
bool isFlashlightOn = true;
bool fKeyWasPressed = false;

ma_engine audioEngine;

int bateriasRecolectadas = 0;
bool hasKeycardYellow = false;
bool hasKeycardRed = false;
bool hasKeycardBlue = false;
bool dimensionAlterna = false;
bool portalActivado = false;
int currentZone = 1;
bool showDebugGUI = false;
bool showCollisionViewer = false;
bool collisionShowWalls = true;
bool collisionShowProps = true;
float collisionViewerRadius = 8.0f;
int selectedHotbarSlot = 0;

GLTFModel *banoGLTF = nullptr;
GLTFModel *lavamanosGLTF = nullptr;
GLTFModel *urinarioGLTF = nullptr;
GLTFModel *mensBGLTF = nullptr;
GLTFModel *girlBGLTF = nullptr;

// area de contencion
GLTFModel *teslaGLTF = nullptr;
GLTFModel *paredesGLTF = nullptr;
GLTFModel *monitorGLTF = nullptr;
GLTFModel *esquinerosGLTF = nullptr;
GLTFModel *generadorGLTF = nullptr;

GLTFModel *lamparaContencionGLTF = nullptr;
GLTFModel *lampara2GLTF = nullptr;
GLTFModel *lampara3GLTF = nullptr;
GLTFModel *emergencyGLTF = nullptr;
GLTFModel *reactorGLTF = nullptr;
GLTFModel *panelControlGLTF = nullptr;
GLTFModel *consolaGLTF = nullptr;
GLTFModel *lamparaReactorGLTF = nullptr;
GLTFModel *sarcofagoGLTF = nullptr;
GLTFModel *cajonesOFGLTF = nullptr;
GLTFModel *gabineteGLTF = nullptr;
GLTFModel *camaraGLTF = nullptr;
GLTFModel *serversGLTF = nullptr;
GLTFModel *terminalGLTF = nullptr;
GLTFModel *boxCloseGLTF = nullptr;
GLTFModel *boxOpenGLTF = nullptr;
GLTFModel *vaultDoorGLTF = nullptr;
GLTFModel *escritorioGLTF = nullptr;
GLTFModel *mesaGLTF = nullptr;
GLTFModel *miniLamparaGLTF = nullptr;
GLTFModel *computerGLTF = nullptr;
GLTFModel *sillaGLTF = nullptr;
GLTFModel *sillasGLTF = nullptr;
GLTFModel *barraGLTF = nullptr;
GLTFModel *logoGLTF = nullptr;
GLTFModel *logo2GLTF = nullptr;
GLTFModel *cablePisoGLTF = nullptr;
GLTFModel *cableTechoGLTF = nullptr;
GLTFModel* barrilGLTF = nullptr;
GLTFModel* capsulasGLTF = nullptr;
GLTFModel* paredCuarGLTF = nullptr;
GLTFModel* barrilesGLTF = nullptr;
GLTFModel* bigtankGLTF = nullptr;
GLTFModel* generadoresGLTF = nullptr;
GLTFModel* subjectGLTF = nullptr;
GLTFModel* sciboxGLTF = nullptr;

GLTFModel *sangrePisoGLTF = nullptr;
std::vector<glm::vec3> sangrePisoPos = {};
std::vector<glm::vec3> sangrePisoRot = {};
std::vector<glm::vec3> sangrePisoScale = {};

GLTFModel *sangrePiso2GLTF = nullptr;
std::vector<glm::vec3> sangrePiso2Pos = {};
std::vector<glm::vec3> sangrePiso2Rot = {};
std::vector<glm::vec3> sangrePiso2Scale = {};

GLTFModel *sangreParedesGLTF = nullptr;
GLTFModel *sangrePared2GLTF = nullptr;
GLTFModel *behindYouGLTF = nullptr;

glm::vec3 behindYouPos = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 behindYouRot = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 behindYouScale = glm::vec3(1.0f, 1.0f, 1.0f);

GLTFModel *warningGLTF = nullptr;
GLTFModel *sillitaGLTF = nullptr;
GLTFModel *maquinaGLTF = nullptr;
GLTFModel *paredGLTF = nullptr;
GLTFModel *deskGLTF = nullptr;
GLTFModel *estanteGLTF = nullptr;
GLTFModel *sofaGLTF = nullptr;
GLTFModel *lockerGLTF = nullptr;
GLTFModel *estantesGLTF = nullptr;
GLTFModel *monitoringGLTF = nullptr;
GLTFModel *morguefridgeGLTF = nullptr;
GLTFModel *refrigeradorGLTF = nullptr;
GLTFModel *camillaGLTF = nullptr;
GLTFModel *muralGLTF = nullptr;
GLTFModel *terminalesGLTF = nullptr;
GLTFModel *esferaGLTF = nullptr;
GLTFModel *bodybagGLTF = nullptr;
GLTFModel *coffinGLTF = nullptr;
GLTFModel *bloodyboxGLTF = nullptr;
GLTFModel *labtableGLTF = nullptr;
GLTFModel *shelfGLTF = nullptr;
GLTFModel *safetyGLTF = nullptr;
GLTFModel* neveraGLTF = nullptr;
GLTFModel* estantebodGLTF = nullptr;
GLTFModel* boxesGLTF = nullptr;

GLTFModel *interruptorGLTF = nullptr;
GLTFModel *ascensorGLTF = nullptr;
GLTFModel *cajaElectricaGLTF = nullptr;
GLTFModel *plataformaGLTF = nullptr;
GLTFModel *ductoGLTF = nullptr;
GLTFModel *ghostGLTF = nullptr;
GLTFModel *headGLTF = nullptr;

glm::vec3 sarcofagoPos(43.235f, -0.100f, 12.691f);
glm::vec3 sarcofagoRot(0.0f, 0.0f, 0.0f);
glm::vec3 sarcofagoScale(1.260f, 1.060f, 0.930f);

glm::vec3 cajonesOFPos(8.0f, -0.5f, 5.0f);
glm::vec3 cajonesOFRot(0.0f, 0.0f, 0.0f);
glm::vec3 cajonesOFScale(1.0f, 1.0f, 1.0f);

std::vector<glm::vec3> cablePisoPos = {glm::vec3(43.235f, -0.100f, 12.691f)};
std::vector<glm::vec3> cablePisoRot = {glm::vec3(0.0f, 0.0f, 0.0f)};
std::vector<glm::vec3> cablePisoScale = {
    glm::vec3(0.01918f, 0.01918f, 0.01918f)};

std::vector<glm::vec3> cableTechoPos = {glm::vec3(43.235f, -0.100f, 12.691f)};
std::vector<glm::vec3> cableTechoRot = {glm::vec3(0.0f, 0.0f, 0.0f)};
std::vector<glm::vec3> cableTechoScale = {
    glm::vec3(-0.001557f, -0.001557f, -0.001557f)};

glm::vec3 warningPos(42.500f, 0.200f, 20.866f);
glm::vec3 warningRot(-89.000f, -180.000f, -0.500f);
glm::vec3 warningScale(1.410f, 0.950f, 1.570f);

glm::vec3 teslaPos(47.950f, -0.500f, 14.400f);
glm::vec3 teslaRot(-88.000f, 0.0f, 0.0f);
glm::vec3 teslaScale(0.150f, 0.120f, 0.090f);

glm::vec3 lamparaContencionPos(47.800f, 1.050f, 15.450f);
glm::vec3 lamparaContencionRot(-88.000f, 0.0f, 89.000f);
glm::vec3 lamparaContencionScale(0.570f, 0.720f, 0.900f);

glm::vec3 lampara2Pos(36.155f, 0.450f, 18.042f);
glm::vec3 lampara2Rot(-89.000f, 0.000f, 89.000f);
glm::vec3 lampara2Scale(1.010f, 8.980f, 1.070f);

glm::vec3 reactorPos(43.163f, -0.550f, 19.211f);
glm::vec3 reactorRot(0.000f, 0.000f, 0.000f);
glm::vec3 reactorScale(1.00f, 1.000f, 1.000f);

glm::vec3 lampara3Pos(36.155f, 0.450f, 16.392f);
glm::vec3 lampara3Rot(-89.000f, 0.000f, 89.000f);
glm::vec3 lampara3Scale(1.010f, 8.980f, 1.070f);

// Luces parpadeantes (estilo baño) para la sala de descanso / sofas
glm::vec3 luzDescanso1Pos(5.000f, 2.500f, 2.000f);
glm::vec3 luzDescanso1Rot(0.0f, -180.0f, 0.0f);
glm::vec3 luzDescanso1Scale(0.520f, 0.490f, 1.0f);
glm::vec3 luzDescanso2Pos(12.000f, 2.500f, 2.000f);
glm::vec3 luzDescanso2Rot(0.0f, -180.0f, 0.0f);
glm::vec3 luzDescanso2Scale(0.520f, 0.490f, 1.0f);

glm::vec3 panelControlPos(48.350f, -0.500f, 17.450f);
glm::vec3 panelControlRot(-91.000f, 0.000f, -180.000f);
glm::vec3 panelControlScale(0.450f, 0.490f, 0.180f);

glm::vec3 consolaPos(40.613f, -0.200, 17.770f);
glm::vec3 consolaRot(-90.000f, 0.000f, -147.500f);
glm::vec3 consolaScale(0.890f, 0.730f, 0.280f);

glm::vec3 lamparaReactorPos(44.000f, 0.350f, 15.157f);
glm::vec3 lamparaReactorRot(-1.000f, 48.500f, -19.500f);
glm::vec3 lamparaReactorScale(1.000f, 1.000f, 1.000f);

glm::vec3 lamparaReactorPos2(43.900f, 0.400f, 17.067f);
glm::vec3 lamparaReactorRot2(0.000f, -50.000f, 0.000f);
glm::vec3 lamparaReactorScale2(1.000f, 1.000f, 1.000f);

glm::vec3 lamparaReactorPos3(42.585f, 0.350f, 16.946f);
glm::vec3 lamparaReactorRot3(0.000f, -154.000f, 0.000f);
glm::vec3 lamparaReactorScale3(1.000f, 1.000f, 1.000f);

glm::vec3 lamparaReactorPos4(42.594f, 0.300f, 15.620f);
glm::vec3 lamparaReactorRot4(0.000f, 149.500f, 0.000f);
glm::vec3 lamparaReactorScale4(1.000f, 1.000f, 1.000f);

glm::vec3 esquinerosPos(48.521f, -0.500f, 12.504f);
glm::vec3 esquinerosRot(0.000f, -52.000f, 0.000f);
glm::vec3 esquinerosScale(0.890f, 0.750f, 0.810f);

glm::vec3 generadorPos[3] = {glm::vec3(36.234f, -0.600f, 15.550f),
                             glm::vec3(36.266f, -0.600f, 17.254f),
                             glm::vec3(36.261f, -0.600f, 18.923f)};
glm::vec3 generadorRot[3] = {glm::vec3(-1.500f, 92.500f, -0.500f),
                             glm::vec3(0.000f, 92.500f, 1.500f),
                             glm::vec3(0.000f, 95.500f, 0.000f)};
glm::vec3 generadorScale[3] = {glm::vec3(1.0f, 1.0f, 1.0f),
                               glm::vec3(1.0f, 1.0f, 1.0f),
                               glm::vec3(1.0f, 1.0f, 1.0f)};

glm::vec3 esquineros2Pos(48.283f, -0.500f, 20.301f);
glm::vec3 esquineros2Rot(-2.500f, -144.000f, -0.500f);
glm::vec3 esquineros2Scale(0.890f, 0.760f, 0.610f);

glm::vec3 esquineros3Pos(34.683f, -0.500f, 12.581f);
glm::vec3 esquineros3Rot(-0.500f, 56.000f, -0.500f);
glm::vec3 esquineros3Scale(0.890f, 0.760f, 0.860f);

glm::vec3 esquineros4Pos(34.833f, -0.550f, 20.451f);
glm::vec3 esquineros4Rot(0.500f, 120.000f, -0.500f);
glm::vec3 esquineros4Scale(0.890f, 0.760f, 0.610f);

// area principal (escenario grande)
GLTFModel *machineLabGLTF = nullptr;
glm::vec3 machineLabPos[3] = {glm::vec3(17.213f, -0.300f, 2.773f),
                              glm::vec3(26.963f, -0.300f, 2.773f),
                              glm::vec3(22.463f, -0.300f, 4.523f)};
glm::vec3 machineLabRot[3] = {glm::vec3(0.000f, 0.000f, 0.000f),
                              glm::vec3(0.000f, 0.000f, 0.000f),
                              glm::vec3(0.000f, 0.000f, 0.000f)};
glm::vec3 machineLabScale[3] = {glm::vec3(1.000f, 1.000f, 1.000f),
                                glm::vec3(1.000f, 1.000f, 1.000f),
                                glm::vec3(1.000f, 1.000f, 1.000f)};

GLTFModel *metalDeskGLTF = nullptr;
glm::vec3 metalDeskPos[8] = {
    glm::vec3(16.500f, -0.300f, 6.000f), glm::vec3(18.000f, -0.300f, 6.000f),
    glm::vec3(19.500f, -0.300f, 6.000f), glm::vec3(21.000f, -0.300f, 6.000f),
    glm::vec3(22.500f, -0.300f, 6.000f), glm::vec3(24.000f, -0.300f, 6.000f),
    glm::vec3(25.500f, -0.300f, 6.000f), glm::vec3(27.000f, -0.300f, 6.000f)};
glm::vec3 metalDeskRot[8] = {
    glm::vec3(0.000f, 0.000f, 0.000f), glm::vec3(0.000f, 0.000f, 0.000f),
    glm::vec3(0.000f, 0.000f, 0.000f), glm::vec3(0.000f, 0.000f, 0.000f),
    glm::vec3(0.000f, 0.000f, 0.000f), glm::vec3(0.000f, 0.000f, 0.000f),
    glm::vec3(0.000f, 0.000f, 0.000f), glm::vec3(0.000f, 0.000f, 0.000f)};
glm::vec3 metalDeskScale[8] = {
    glm::vec3(0.880f, 0.460f, 1.000f), glm::vec3(0.880f, 0.460f, 1.000f),
    glm::vec3(0.880f, 0.460f, 1.000f), glm::vec3(0.880f, 0.460f, 1.000f),
    glm::vec3(0.880f, 0.460f, 1.000f), glm::vec3(0.880f, 0.460f, 1.000f),
    glm::vec3(0.880f, 0.460f, 1.000f), glm::vec3(0.880f, 0.460f, 1.000f)};

std::vector<Entity> gameEntities = {
    {glm::vec3(8.0f, -0.4f, 4.0f), 3, true, "ENTITY_WIRE", 0.0f},
    {glm::vec3(20.0f, -0.4f, 5.0f), 0, true, "ENTITY_LOG1", 0.0f},
    {glm::vec3(15.0f, -0.4f, 3.0f), 8, true, "", 0.0f}, // Tarjeta Amarilla (Pruebas)
    {glm::vec3(12.0f, -0.2f, 6.0f), 1, true, "", 0.0f},
    {glm::vec3(24.0f, -0.5f, 6.0f), 4, true, "", 1.0f},
    {glm::vec3(24.0f, 0.0f, 6.0f), 5, true, "ENTITY_MONITOR_AUX", 1.5f},
    {glm::vec3(10.0f, -0.5f, 15.0f), 4, true, "", 2.0f},
    {glm::vec3(10.0f, 0.0f, 15.0f), 5, true, "ENTITY_ERROR_SCREEN", 2.5f},
    {glm::vec3(28.0f, 0.0f, 16.0f), 6, true, "ENTITY_MACHINE", 5.0f},
    {glm::vec3(40.6f, -0.4f, 17.0f), 9, true, "", 0.0f}, // Tarjeta Roja (Contencion)
    {glm::vec3(42.0f, -0.2f, 15.0f), 0, true, "ENTITY_LOG2", 0.0f},
    {glm::vec3(15.0f, -0.2f, 18.0f), 1, true, "", 0.0f},
    {glm::vec3(10.0f, -0.4f, 28.0f), 3, true, "ENTITY_STAIN", 0.0f},
    {glm::vec3(25.0f, 0.0f, 3.0f), 2, true, "", 0.0f},
    {glm::vec3(5.0f, -0.4f, 13.5f), 11, true, "", 0.0f}}; // Tarjeta Azul (Bodega)

float wallWidth = 0.3f;
float wallHeight = 1.0f;
float door1Anim = 0.0f;
bool door1Opening = false;
float door2Anim = 0.0f;
bool door2Opening = false;
std::map<int, float> activeDoorsAnim;

// Wire puzzle state
bool wirePuzzleActive = false;
bool resetWirePuzzle = false;
int blackDoorGridX = -1;
int blackDoorGridZ = -1;

bool symbolPuzzleActive = false;
int whiteDoorGridX = -1;
int whiteDoorGridZ = -1;
int symbolPuzzleTargetSymbol = 0;
int symbolPuzzleWheelIndices[3] = { 0, 0, 0 };

float gameTimer = 600.0f; // 10 minutes in seconds
bool switch1Active = false;
bool switch2Active = false;
bool switch3Active = false;
bool switch1Solved = false;
bool switch2Solved = false;
bool switch3Solved = false;
float switch1Lever = 0.0f;
float switch2Lever = 0.0f;
float switch3Lever = 0.0f;
bool allLightsOn = false;
bool gameWon = false;

int worldMap[MAP_HEIGHT][MAP_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
     0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
     0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 11, 11, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 7, 7, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 7, 7, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 10, 10, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 8, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 7, 7, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4},
    {1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 11, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 11, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 7, 7, 4, 4, 4, 4, 4, 4},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 12, 12, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

std::vector<PlacedProp> placedProps;
std::map<std::string, GLTFModel *> modelRegistry;

void saveLevelProps(const std::string &path) {
  std::ofstream outFile(path);
  if (!outFile.is_open()) {
    std::cerr << "Error al abrir el archivo para guardar: " << path
              << std::endl;
    return;
  }
  for (const auto &prop : placedProps) {
    outFile << prop.modelName << " " << prop.pos.x << " " << prop.pos.y << " "
            << prop.pos.z << " " << prop.rot.x << " " << prop.rot.y << " "
            << prop.rot.z << " " << prop.scale.x << " " << prop.scale.y << " "
            << prop.scale.z << " " << (prop.collisionActive ? 1 : 0) << " "
            << prop.area << "\n";
  }
  outFile.close();
  std::cout << "Mapa guardado exitosamente en: " << path << std::endl;
}

void loadLevelProps(const std::string &path) {
  placedProps.clear();
  std::ifstream inFile(path);
  if (inFile.is_open()) {
    std::string line;
    while (std::getline(inFile, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      std::stringstream ss(line);
      PlacedProp prop;
      prop.collisionActive = true;
      if (ss >> prop.modelName >> prop.pos.x >> prop.pos.y >> prop.pos.z >>
          prop.rot.x >> prop.rot.y >> prop.rot.z >> prop.scale.x >>
          prop.scale.y >> prop.scale.z) {
        int activeVal = 1;
        if (ss >> activeVal) {
          prop.collisionActive = (activeVal != 0);
        }
        // Retrocompatible: si no hay area en el archivo, inferirla del modelo
        std::string areaStr;
        if (ss >> areaStr) {
          prop.area = areaStr;
        } else {
          prop.area = getModelArea(prop.modelName);
        }
        placedProps.push_back(prop);
      }
    }
    inFile.close();
    std::cout << "Cargados " << placedProps.size() << " props desde: " << path
              << std::endl;

    // Migración de Baño: Si no existe MirrorBG, añadimos todos los props del
    // baño
    bool hasBano = false;
    for (const auto &p : placedProps) {
      if (p.modelName == "MirrorBG") {
        hasBano = true;
        break;
      }
    }
    if (!hasBano) {
      placedProps.push_back(
          {"MirrorBG", mirrorBGpos, mirrorBGRot, mirrorBGScale, true, "Baño"});
      placedProps.push_back(
          {"mensB", mensBpos, mensBrot, mensBscale, true, "Baño"});
      placedProps.push_back(
          {"girlB", girlBpos, girlBrot, girlBscale, true, "Baño"});
      placedProps.push_back(
          {"mirror", mirrorPos, mirrorRot, mirrorScale, true, "Baño"});
      placedProps.push_back(
          {"mirror", mirrorPos2, mirrorRot2, mirrorScale2, true, "Baño"});
      placedProps.push_back(
          {"mirror", mirrorPos3, mirrorRot3, mirrorScale3, true, "Baño"});
      placedProps.push_back(
          {"mirror", mirrorPos4, mirrorRot4, mirrorScale4, true, "Baño"});
      placedProps.push_back({"ligthbathroom", ligthbathroomPos,
                             ligthbathroomRot, ligthbathroomScale, true,
                             "Baño"});
      placedProps.push_back(
          {"ligthbathroom", lamp3Pos, lamp3Rot, lamp3Scale, true, "Baño"});
      placedProps.push_back(
          {"ligthbathroom", lamp4Pos, lamp4Rot, lamp4Scale, true, "Baño"});
      placedProps.push_back({"ligthbathroom", ligthbathroom2Pos,
                             ligthbathroom2Rot, ligthbathroom2Scale, true,
                             "Baño"});
      placedProps.push_back(
          {"Bano", banoPos, banoRot, banoScale, true, "Baño"});
      placedProps.push_back(
          {"Bano", banoPos2, banoRot2, banoScale2, true, "Baño"});
      placedProps.push_back(
          {"Bano", banoPos3, banoRot3, banoScale3, true, "Baño"});
      placedProps.push_back(
          {"Bano", banoPos4, banoRot4, banoScale4, true, "Baño"});
      placedProps.push_back(
          {"Bano", banoPos5, banoRot5, banoScale5, true, "Baño"});
      placedProps.push_back(
          {"Bano", banoPos6, banoRot6, banoScale6, true, "Baño"});
      placedProps.push_back(
          {"Bano", banoPos7, banoRot7, banoScale7, true, "Baño"});
      placedProps.push_back(
          {"Bano", banoPos8, banoRot8, banoScale8, true, "Baño"});
      placedProps.push_back({"lavamanos", lavamanosPos, lavamanosRot,
                             lavamanosScale, true, "Baño"});
      placedProps.push_back({"lavamanos", lavamanosPos2, lavamanosRot2,
                             lavamanosScale2, true, "Baño"});
      placedProps.push_back({"lavamanos", lavamanosPos3, lavamanosRot3,
                             lavamanosScale3, true, "Baño"});
      placedProps.push_back({"lavamanos", lavamanosPos4, lavamanosRot4,
                             lavamanosScale4, true, "Baño"});
      placedProps.push_back({"lavamanos", lavamanosPos5, lavamanosRot5,
                             lavamanosScale5, true, "Baño"});
      placedProps.push_back({"lavamanos", lavamanosPos6, lavamanosRot6,
                             lavamanosScale6, true, "Baño"});
      placedProps.push_back({"lavamanos", lavamanosPos7, lavamanosRot7,
                             lavamanosScale7, true, "Baño"});
      placedProps.push_back({"lavamanos", lavamanosPos8, lavamanosRot8,
                             lavamanosScale8, true, "Baño"});
      placedProps.push_back(
          {"urinario", urinarioPos, urinarioRot, urinarioScale, true, "Baño"});
      std::cout << "Migracion: Modelos de bano anadidos a placedProps."
                << std::endl;
      saveLevelProps(path);
    }

  } else {
    std::cout << "Archivo de posiciones no encontrado, cargando valores por "
                 "defecto..."
              << std::endl;
    placedProps.push_back({"cajonesOF", glm::vec3(8.0f, -0.5f, 5.0f),
                           glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(1.0f, 1.0f, 1.0f), false, "Oficinas"});
    placedProps.push_back({"sarcofago", glm::vec3(43.235f, -0.100f, 12.691f),
                           glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(1.260f, 1.060f, 0.930f), true,
                           "Contencion"});
    placedProps.push_back({"warning", glm::vec3(39.993f, 0.200f, 20.866f),
                           glm::vec3(-89.000f, -180.000f, -0.500f),
                           glm::vec3(1.410f, 0.950f, 1.570f), true,
                           "Contencion"});
    placedProps.push_back({"tesla", glm::vec3(47.950f, -0.500f, 14.400f),
                           glm::vec3(-88.000f, 0.0f, 0.0f),
                           glm::vec3(0.150f, 0.120f, 0.090f), true,
                           "Contencion"});
    placedProps.push_back({"reactor", glm::vec3(43.163f, -0.550f, 16.111f),
                           glm::vec3(0.000f, 0.000f, 0.000f),
                           glm::vec3(1.00f, 1.000f, 1.000f), true,
                           "Contencion"});
    placedProps.push_back({"panelControl", glm::vec3(48.350f, -0.500f, 17.450f),
                           glm::vec3(-91.000f, 0.000f, -180.000f),
                           glm::vec3(0.450f, 0.490f, 0.180f), true,
                           "Contencion"});
    placedProps.push_back({"consola", glm::vec3(40.613f, -0.200, 17.770f),
                           glm::vec3(-90.000f, 0.000f, -147.500f),
                           glm::vec3(0.890f, 0.730f, 0.280f), true,
                           "Contencion"});
    placedProps.push_back({"esquineros", glm::vec3(48.521f, -0.500f, 12.504f),
                           glm::vec3(0.000f, -52.000f, 0.000f),
                           glm::vec3(0.890f, 0.750f, 0.810f), true,
                           "Contencion"});
    placedProps.push_back({"esquineros2", glm::vec3(48.283f, -0.500f, 20.301f),
                           glm::vec3(-2.500f, -144.000f, -0.500f),
                           glm::vec3(0.890f, 0.760f, 0.610f), true,
                           "Contencion"});
    placedProps.push_back({"esquineros3", glm::vec3(34.683f, -0.500f, 12.581f),
                           glm::vec3(-0.500f, 56.000f, -0.500f),
                           glm::vec3(0.890f, 0.760f, 0.860f), true,
                           "Contencion"});
    placedProps.push_back({"esquineros4", glm::vec3(34.833f, -0.550f, 20.451f),
                           glm::vec3(0.500f, 120.000f, -0.500f),
                           glm::vec3(0.890f, 0.760f, 0.610f), true,
                           "Contencion"});
    placedProps.push_back(
        {"lampara-reactor", glm::vec3(44.000f, 0.350f, 15.157f),
         glm::vec3(-1.000f, 48.500f, -19.500f),
         glm::vec3(1.000f, 1.000f, 1.000f), false, "Contencion"});
    placedProps.push_back(
        {"lampara-reactor", glm::vec3(43.900f, 0.400f, 17.067f),
         glm::vec3(0.000f, -50.000f, 0.000f), glm::vec3(1.000f, 1.000f, 1.000f),
         false, "Contencion"});
    placedProps.push_back(
        {"lampara-reactor", glm::vec3(42.585f, 0.350f, 16.946f),
         glm::vec3(0.000f, -154.000f, 0.000f),
         glm::vec3(1.000f, 1.000f, 1.000f), false, "Contencion"});
    placedProps.push_back(
        {"lampara-reactor", glm::vec3(42.594f, 0.300f, 15.620f),
         glm::vec3(0.000f, 149.500f, 0.000f), glm::vec3(1.000f, 1.000f, 1.000f),
         false, "Contencion"});

    // Luces de emergencia migradas al sistema de props
    placedProps.push_back({"emergency", glm::vec3(39.693f, 0.400f, 20.935f),
                           glm::vec3(-93.500f, 0.000f, 0.000f),
                           glm::vec3(1.0f, 1.0f, 1.0f), true, "Contencion"});
    placedProps.push_back({"emergency", glm::vec3(39.597f, 0.274f, 11.987f),
                           glm::vec3(93.500f, 2.500f, 88.000f),
                           glm::vec3(0.990f, 0.980f, 1.0f), true,
                           "Contencion"});
    placedProps.push_back({"emergency", glm::vec3(3.0f, 2.5f, 3.0f),
                           glm::vec3(0.000f, 0.000f, 0.000f),
                           glm::vec3(1.0f, 1.0f, 1.0f), true, "Descanso"});
    placedProps.push_back({"emergency", glm::vec3(10.0f, 2.5f, 3.0f),
                           glm::vec3(0.000f, 0.000f, 0.000f),
                           glm::vec3(1.0f, 1.0f, 1.0f), true, "Descanso"});

    // Switches (caja-electrica)
    placedProps.push_back({"caja-electrica", glm::vec3(5.0f, -0.4f, 14.0f),
                           glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(1.0f, 1.0f, 1.0f), true, "Bodega"});
    placedProps.push_back({"caja-electrica", glm::vec3(45.0f, -0.4f, 29.0f),
                           glm::vec3(0.0f, -90.0f, 0.0f),
                           glm::vec3(1.0f, 1.0f, 1.0f), true, "Ascensor"});
    placedProps.push_back({"caja-electrica", glm::vec3(26.0f, -0.4f, 15.0f),
                           glm::vec3(0.0f, 180.0f, 0.0f),
                           glm::vec3(1.0f, 1.0f, 1.0f), true, "Muestras"});

    saveLevelProps(path);
  }
}
